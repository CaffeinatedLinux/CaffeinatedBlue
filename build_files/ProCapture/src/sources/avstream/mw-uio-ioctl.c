#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/version.h>

#include <xi-version.h>
#include "dma/mw-dma-mem-priv.h"
#include "ospi/linux-file.h"

#include "xi-driver.h"
#include "mw-uio-drv.h"
#include "ospi/ospi.h"
#include "v4l2.h"
#include "alsa.h"
#include "capture.h"
#include "mw-event-dev.h"
#include "xi-driver-priv.h"
#include "front-end/front-end.h"
#include "mw-fe-uapi.h"
#include "mw-netlink.h"
#include "drivers/i2c-master.h"
#include "drivers/irq-control.h"

static struct xi_front_end **mw_fe_uio_dev_to_front_end(struct uio_device *idev)
{
	struct xi_cap_context *ctx;
	struct xi_front_end **fes;

	ctx = idev->info->priv;
	if (!ctx)
		return NULL;

	fes = xi_driver_get_front_end(&ctx->driver);
	if (!fes)
		return NULL;

	return fes;
}

static struct xi_driver *mw_fe_uio_dev_to_driver(struct uio_device *idev)
{
	struct xi_cap_context *ctx;

	ctx = idev->info->priv;
	if (!ctx)
		return NULL;

	return &ctx->driver;
}

static void front_end_SetInputSpecificStatus(struct xi_front_end *front,
		INPUT_SPECIFIC_STATUS *pStatus)
{
	os_spin_lock(front->m_lockStatus);
	front->m_inputSpecificStatus = *pStatus;
	os_spin_unlock(front->m_lockStatus);

	xi_debug(5, "notify Specific signal changed\n");
	// Notify subscriptors
	xi_front_end_Notify(front, MWCAP_NOTIFY_INPUT_SPECIFIC_CHANGE);
}

static void front_end_SetVideoSignalStatus(struct xi_front_end *front,
		VIDEO_SIGNAL_STATUS *pStatus)
{
	VIDEO_CAPTURE_SETTINGS videoCaptureSettings;
	ULONGLONG ullNotifyMasks;

	xi_debug(5, "pubStatus.state:%d\n", pStatus->pubStatus.state);
	videoCaptureSettings.bValid = (pStatus->pubStatus.state >= MWCAP_VIDEO_SIGNAL_LOCKING);
	videoCaptureSettings.x = pStatus->pubStatus.x;
	videoCaptureSettings.y = pStatus->pubStatus.y;
	videoCaptureSettings.cx = pStatus->pubStatus.cx;
	videoCaptureSettings.cy = pStatus->pubStatus.cy;
	xi_debug(5, "x:%d,y:%d,cx:%d,cy:%d\n",
			videoCaptureSettings.x, videoCaptureSettings.y, videoCaptureSettings.cx, videoCaptureSettings.cy);
	videoCaptureSettings.nAspectX = pStatus->nEffectiveAspectX;
	videoCaptureSettings.nAspectY = pStatus->nEffectiveAspectY;
	xi_debug(5, "nAspectX:%d,nAspectY:%d\n", videoCaptureSettings.nAspectX, videoCaptureSettings.nAspectY);
	videoCaptureSettings.bInterlaced = pStatus->pubStatus.bInterlaced;
	videoCaptureSettings.bSegmentedFrame = pStatus->pubStatus.bSegmentedFrame;
	videoCaptureSettings.byInputSelect = pStatus->byInputSelect;
	videoCaptureSettings.bFramePaking = pStatus->bFramePacking;
	videoCaptureSettings.yRightFrame = pStatus->yRightFrame;
	videoCaptureSettings.bInterlacedFramePacking = pStatus->bInterlacedFramePacking;
	videoCaptureSettings.cyInterlacedFramePacking = pStatus->cyInterlacedFramePacking;

	videoCaptureSettings.colorFormat = pStatus->nEffectiveColorFormat;
	videoCaptureSettings.quantRange = pStatus->nEffectiveQuantRange;
	videoCaptureSettings.sRVBUScale = pStatus->sRVBUScale;
	videoCaptureSettings.sRVBUOffset = pStatus->sRVBUOffset;

	xi_debug(5, "colorFormat:%d\n", pStatus->nEffectiveColorFormat);
	ullNotifyMasks = MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE;
	os_spin_lock(front->m_lockStatus);
	front->m_videoSignalStatus = *pStatus;

	if (xi_front_end_IsVideoCaptureSettingsChanged(front, &videoCaptureSettings)) {
		ullNotifyMasks |= NOTIFY_EVENT_VIDEO_CAPTURE_SETTINGS_CHANGE;
		front->m_videoCaptureSettings = videoCaptureSettings;
	}

	os_spin_unlock(front->m_lockStatus);

	xi_debug(5, "notify Video signal changed\n");
	// Notify subscriptors
	xi_front_end_Notify(front, ullNotifyMasks);
}

static void front_end_SetAudioSignalStatus(struct xi_front_end *front,
		AUDIO_SIGNAL_STATUS *pStatus, BOOLEAN bNotifyChangeInputSource)
{
	AUDIO_CAPTURE_SETTINGS audioCaptureSettings;
	ULONGLONG ullNotifyMasks;

	audioCaptureSettings.bFreeRun = pStatus->bFreeRun;
	audioCaptureSettings.byInputSelect = pStatus->byInputSelect;
	audioCaptureSettings.audioInputMode = pStatus->audioInputMode;
	audioCaptureSettings.bLeftAlign = pStatus->bLeftAlign;
	audioCaptureSettings.bMSBFirst = pStatus->bMSBFirst;
	audioCaptureSettings.byMSBIndex = pStatus->byMSBIndex;
	audioCaptureSettings.wChannelValid = pStatus->pubStatus.wChannelValid;
	audioCaptureSettings.dwSampleRate = pStatus->pubStatus.dwSampleRate;

	ullNotifyMasks = bNotifyChangeInputSource ? MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE : 0;
	ullNotifyMasks |= MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE;

	os_spin_lock(front->m_lockStatus);
	front->m_audioSignalStatus = *pStatus;

	if (xi_front_end_IsAudioCaptureSettingsChanged(front, &audioCaptureSettings)) {
		ullNotifyMasks |= NOTIFY_EVENT_AUDIO_CAPTURE_SETTINGS_CHANGE;
		front->m_audioCaptureSettings = audioCaptureSettings;
	}
	os_spin_unlock(front->m_lockStatus);
	xi_debug(5, "notify Audio signal changed\n");
	// Notify subscriptors
	xi_front_end_Notify(front, ullNotifyMasks);
}

static int mw_fe_signal_status_changed(struct uio_device *idev, struct mw_fe_signal_status *status)
{
	struct xi_front_end **fes = mw_fe_uio_dev_to_front_end(idev);

	if (!fes)
		return -EINVAL;

	if (status->chn < 0 || status->chn >= MAX_CHANNELS_PER_DEVICE)
		return -EINVAL;

	xi_debug(5, "chn:%d\n", status->chn);
	xi_debug(5, "ullNotifyBits:0x%llx\n", status->ullNotifyBits);
	// Save changes
	if (status->ullNotifyBits & MWCAP_NOTIFY_INPUT_SPECIFIC_CHANGE)
		front_end_SetInputSpecificStatus(fes[status->chn], &status->m_inputSpecificStatus);

	if (status->ullNotifyBits & MWCAP_NOTIFY_VIDEO_SIGNAL_CHANGE)
		front_end_SetVideoSignalStatus(fes[status->chn], &status->m_videoSignalStatus);

	if (status->ullNotifyBits & MWCAP_NOTIFY_AUDIO_SIGNAL_CHANGE)
		front_end_SetAudioSignalStatus(fes[status->chn], &status->m_audioSignalStatus, FALSE);

	if (status->ullNotifyBits & MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE)
		xi_front_end_Notify(fes[status->chn], MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE);

	return 0;
}


static int mw_fe_get_device_info(struct uio_device *idev, struct mw_board_info *info)
{
	struct cap_device_info *cap_info = idev->info;
	struct device_mem *mem = &cap_info->mem;
	struct xi_driver *driver = mw_fe_uio_dev_to_driver(idev);
	int ret;

	if (!mem || !driver)
		return -EINVAL;

	info->map_size = mem->size;

	ret = xi_driver_get_device_info(driver, info);
	if (ret < 0)
		return ret;

	xi_debug(5, "map_size:%d\n", info->map_size);
	return 0;
}

static int _mw_fe_sync_input_source_from_user(struct xi_front_end *front, struct front_end_input_source *inputSource)
{
	if (!front)
		return -EINVAL;

	if (inputSource->numVideoInputSources > MAX_INPUT_SOURCE ||
			inputSource->numAudioInputSources > MAX_INPUT_SOURCE)
		return -EINVAL;

	os_memcpy(&front->inputSource, inputSource, sizeof(front->inputSource));

	xi_debug(20, "videoSupportInputSources0:%x,videoSupportInputSources1:%x\n", inputSource->videoSupportInputSources[0], inputSource->videoSupportInputSources[1]);

	return 0;
}

static int _mw_fe_sync_video_input_source_from_user(struct xi_front_end *front, DWORD videoInputSource)
{
	if (!front)
		return -EINVAL;

	front->inputSource.videoInputSource = videoInputSource;

	return 0;
}

static int _mw_fe_sync_audio_input_source_from_user(struct xi_front_end *front, DWORD audioInputSource)
{
	if (!front)
		return -EINVAL;

	front->inputSource.audioInputSource = audioInputSource;

	return 0;
}

static int mw_fe_sync_input_source_from_user(struct uio_device *idev, unsigned long arg)
{
	struct front_end_input_source inputSource;
	struct xi_front_end **fes = mw_fe_uio_dev_to_front_end(idev);
	int ret = 0;

	if (copy_from_user(&inputSource, (struct front_end_input_source __user *)arg, sizeof(inputSource)))
		return -EFAULT;

	/*init input souce*/
	if (inputSource.ullNotifyBits & (MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE | MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE)) {
		ret = _mw_fe_sync_input_source_from_user(fes[0], &inputSource);
		return ret;
	}

	if (inputSource.ullNotifyBits & MWCAP_NOTIFY_VIDEO_INPUT_SOURCE_CHANGE) {
		ret = _mw_fe_sync_video_input_source_from_user(fes[0], inputSource.videoInputSource);
		if (ret)
			return ret;
	}

	if (inputSource.ullNotifyBits & MWCAP_NOTIFY_AUDIO_INPUT_SOURCE_CHANGE)
		ret = _mw_fe_sync_audio_input_source_from_user(fes[0], inputSource.audioInputSource);

	return ret;
}

static int _mw_fe_sync_volume_from_user(struct xi_front_end *front, struct front_end_volume_info *volume_info)
{
	if (!front || !volume_info)
		return -EINVAL;

	front->volume[volume_info->chn_index] = volume_info->volume;

	return 0;
}

static int mw_fe_sync_volume_from_user(struct uio_device *idev, unsigned long arg)
{
	struct front_end_volume_info volume_info;
	struct xi_front_end **fes = mw_fe_uio_dev_to_front_end(idev);

	if (copy_from_user(&volume_info, (struct front_end_volume_info __user *)arg, sizeof(volume_info)))
		return -EFAULT;

	return _mw_fe_sync_volume_from_user(fes[0], &volume_info);
}

static void mw_user_get_device_params(struct device_params *dev_pars, struct parameter_t *par)
{
	dev_pars->m_nEDIDType = (MW_PARAMETER_EDID_TYPE)par->m_nEDIDType;
	dev_pars->m_bEnableHDCP = par->m_bEnableHDCP;
	dev_pars->m_byEDIDMode = par->m_byEDIDMode;
	os_memcpy(dev_pars->m_abyEDID, par->m_abyEDID, 256);
	dev_pars->m_cbEDID = par->m_cbEDID;
	dev_pars->m_dwFrameDurationChangeThreshold = par->m_dwFrameDurationChangeThreshold;
	dev_pars->m_dwVideoSyncSliceLevel = par->m_dwVideoSyncSliceLevel;
	dev_pars->m_bInputSourceScan = par->m_bInputSourceScan;
	dev_pars->m_dwScanPeriod = par->m_dwScanPeriod;
	dev_pars->m_dwScanKeepDuration = par->m_dwScanKeepDuration;
	dev_pars->m_bAVInputSourceLink = par->m_bAVInputSourceLink;
	dev_pars->m_dwVideoInputSource = par->m_dwVideoInputSource;
	dev_pars->m_dwAudioInputSource = par->m_dwAudioInputSource;
	dev_pars->m_bAutoHAlign = par->m_bAutoHAlign;
	dev_pars->m_bySamplingPhase = par->m_bySamplingPhase;
	dev_pars->m_bAutoSamplingPhaseAdjust = par->m_bAutoSamplingPhaseAdjust;
	dev_pars->m_nVideoInputAspectX = par->m_nVideoInputAspectX;
	dev_pars->m_nVideoInputAspectY = par->m_nVideoInputAspectY;
	dev_pars->m_quantRangeVideoInput = (MW_CAP_VIDEO_QUANTIZATION_RANGE)par->m_quantRangeVideoInput;
	dev_pars->m_byVADThreshold = par->m_byVADThreshold;
}

static int mw_device_irq_ctl(struct uio_device *idev, unsigned long arg)
{
	struct xi_driver *driver = mw_fe_uio_dev_to_driver(idev);
	struct mw_irq_ctl ctl;
	int ret = 0;

	if (copy_from_user(&ctl, (struct mw_irq_ctl __user *)arg, sizeof(ctl)))
		return -EFAULT;

	xi_debug(20, "intSource:%d,cmd:%d\n", ctl.intSource, ctl.cmd);

	switch (ctl.cmd) {
	case IRQ_CMD_ENABLE:
		ret = xi_driver_set_enable_irq(driver, ctl.intSource);
		break;
	case IRQ_CMD_DISABLE:
		ret = xi_driver_set_disable_irq(driver, ctl.intSource);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int mw_notify_event_manager(struct uio_device *idev, unsigned long long flags)
{
	struct xi_front_end **fes = mw_fe_uio_dev_to_front_end(idev);

	if (fes[0] == NULL) {
		xi_debug(0, "notify event manager failed,flags:0x%x\n", flags);
		return -EINVAL;
	}

	xi_notify_event_manager_notify(fes[0]->m_pNotifyEventManager, flags);

	return 0;
}

long mw_uio_dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	struct xi_cap_context *ctx;
	struct uio_device *idev = filep->private_data;
	struct xi_driver *driver = mw_fe_uio_dev_to_driver(idev);
	struct mw_fe_signal_status status;
	struct mw_board_info board_info;
	struct mw_i2c_wr_reg_params i2c_params;
	struct mw_netlink_info net_info;
	struct device_params *dev_pars = NULL;
	struct crypto_info cry_info;
	unsigned long long flags;
	long ret = 0;

	ctx = idev->info->priv;
	if (!ctx)
		return -EINVAL;

	switch (cmd) {
	case MW_IOCTL_GET_DEVICE_INFO:
		ret = mw_fe_get_device_info(idev, &board_info);
		if (ret < 0)
			break;

		if (copy_to_user((void __user *)arg, &board_info, sizeof(board_info))) {
			ret = -EFAULT;
			break;
		}
		break;
	case MW_IOCTL_FE_SYNC_SIGNAL_STATUS:

		if (copy_from_user(&status, (struct mw_fe_signal_status __user *)arg, sizeof(status))) {
			ret = -EFAULT;
			break;
		}

		mw_fe_signal_status_changed(idev, &status);
		break;
	case MW_IOCTL_CREATE_NETLINK:
		if (copy_from_user(&net_info, (struct mw_netlink_info __user *)arg, sizeof(net_info))) {
			ret = -EFAULT;
			break;
		}

		xi_debug(5, "user_pid:%d\n", net_info.user_pid);
		idev->fe_sock = mw_uio_dev_manager_get_sock();
		if (!idev->fe_sock) {
			xi_debug(1, "fe_sock null\n");
			ret = -EFAULT;
			break;
		}

		net_info.netlink_id = UIO_NETLINK_ID;
		xi_debug(5, "netlink_id:%d\n", net_info.netlink_id);

		if (copy_to_user((void __user *)arg, &net_info, sizeof(net_info))) {
			xi_debug(1, "failed copy to user\n");
			ret = -EFAULT;
			break;
		}

		idev->netlink_id = net_info.netlink_id;
		idev->user_pid = net_info.user_pid;

		break;

	case MW_IOCTL_I2C_READ_REG:
		if (copy_from_user(&i2c_params, (struct mw_i2c_wr_reg_params __user *)arg, sizeof(i2c_params))) {
			ret = -EFAULT;
			break;
		}

		ret = xi_driver_i2c_read_regs(&ctx->driver, i2c_params.ch, i2c_params.devaddr, i2c_params.regaddr, i2c_params.val, i2c_params.count);
		if (ret == 0) {
			ret = -EFAULT;
			break;
		}

		if (copy_to_user((void __user *)arg, &i2c_params, sizeof(i2c_params))) {
			ret = -EFAULT;
			break;
		}
		break;
	case MW_IOCTL_I2C_READ:
		if (copy_from_user(&i2c_params, (struct mw_i2c_wr_reg_params __user *)arg, sizeof(i2c_params))) {
			ret = -EFAULT;
			break;
		}

		ret = xi_i2c_master_read(xi_driver_get_i2c_master(driver), i2c_params.ch, i2c_params.devaddr, i2c_params.regaddr, i2c_params.val);

		if (copy_to_user((void __user *)arg, &i2c_params, sizeof(i2c_params))) {
			ret = -EFAULT;
			break;
		}
		break;
	case MW_IOCTL_I2C_WRITE_REG:
		if (copy_from_user(&i2c_params, (struct mw_i2c_wr_reg_params __user *)arg, sizeof(i2c_params))) {
			ret = -EFAULT;
			break;
		}

		xi_debug(20, "devaddr:%x,regaddr:%x,ch:%d,val:%x,count:%d\n", i2c_params.devaddr, i2c_params.regaddr, i2c_params.ch, i2c_params.val[0], i2c_params.count);
		ret = xi_driver_i2c_write_regs(&ctx->driver, i2c_params.ch, i2c_params.devaddr, i2c_params.regaddr, i2c_params.val, i2c_params.count);
		xi_debug(20, "end write i2c reg ret:%d\n", ret);

		break;
	case MW_IOCTL_I2C_WRITE:
		if (copy_from_user(&i2c_params, (struct mw_i2c_wr_reg_params __user *)arg, sizeof(i2c_params))) {
			ret = -EFAULT;
			break;
		}

		ret = xi_i2c_master_write(xi_driver_get_i2c_master(driver), i2c_params.ch, i2c_params.devaddr, i2c_params.regaddr, i2c_params.val[0]);
		xi_debug(20, "end write i2c ret:%d\n", ret);
		break;
	case MW_IOCTL_GET_BOARD_PARAMS:
		dev_pars = os_zalloc(sizeof(*dev_pars));
		if (!dev_pars) {
			ret = -ENOMEM;
			break;
		}

		mw_user_get_device_params(dev_pars, xi_driver_get_parameter_manager(driver));

		if (copy_to_user((void __user *)arg, dev_pars, sizeof(*dev_pars))) {
			ret = -EFAULT;
			break;
		}
		break;

	case MW_IOCTL_SYNC_CRYPTO_SERIAL:
		if (copy_from_user(&cry_info, (struct crypto_info __user *)arg, sizeof(cry_info))) {
			ret = -EFAULT;
			break;
		}

		xi_driver_set_crypto_serial(driver, cry_info.serial);

		xi_debug(5, "szBoardSerialNo:%s\n", cry_info.serial);
		break;

	case MW_IOCTL_NOTIFY_EVENT_MANAGER:
		if (copy_from_user(&flags, (unsigned long long __user *)arg, sizeof(flags))) {
			ret = -EFAULT;
			break;
		}
		ret = mw_notify_event_manager(idev, flags);
		break;
	case MW_IOCTL_FE_SYNC_INPUT_SOURCE:
		ret = mw_fe_sync_input_source_from_user(idev, arg);
		break;
	case MW_IOCTL_FE_SYNC_VOLUME:
		ret = mw_fe_sync_volume_from_user(idev, arg);
		break;
	case MW_IOCTL_NOTIFY_IRQ_CTL:
		ret = mw_device_irq_ctl(idev, arg);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	if (dev_pars)
		os_free(dev_pars);

	return ret;
}
