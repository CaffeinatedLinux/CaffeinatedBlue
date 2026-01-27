////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "pcie-dma-desc-chain.h"
#include "mw-fourcc.h"

int xi_pcie_dma_desc_chain_init(struct pcie_dma_desc_chain *pobj, u32 max_items, os_dma_par_t par)
{
    pobj->max_items = 0;

    pobj->addr_desc = os_contig_dma_alloc(sizeof(PCIE_DMA_DESCRIPTOR) * max_items, par);
    if (pobj->addr_desc == NULL)
        return OS_RETURN_NOMEM;

    pobj->max_items = max_items;
    pobj->num_items = 0;

    return OS_RETURN_SUCCESS;
}

void xi_pcie_dma_desc_chain_deinit(struct pcie_dma_desc_chain *pobj)
{
    if (NULL != pobj) {
        if (pobj->addr_desc != NULL)
            os_contig_dma_free(pobj->addr_desc);
        pobj->addr_desc = NULL;
        pobj->max_items = 0;
        pobj->num_items = 0;
    }
}

physical_addr_t xi_pcie_dma_desc_chain_get_address(struct pcie_dma_desc_chain *pobj)
{
    physical_addr_t addr;
    mw_physical_addr_t desc_phys = os_contig_dma_get_phys(pobj->addr_desc);

    addr.addr_high = sizeof(desc_phys) > 4 ? (desc_phys >> 32) & 0xFFFFFFFF : 0;
    addr.addr_low  = desc_phys & 0xFFFFFFFF;

    return addr;
}

int xi_pcie_dma_desc_chain_set_num_items(struct pcie_dma_desc_chain *pobj, u32 items, bool ring_buf)
{
    mw_physical_addr_t desc_phys = os_contig_dma_get_phys(pobj->addr_desc);

    pobj->num_items = items;

    if (ring_buf) {
        PCIE_DMA_DESCRIPTOR * pitems =
                (PCIE_DMA_DESCRIPTOR *)(os_contig_dma_get_virt(pobj->addr_desc));

        pitems[items-1].dwHostAddrHigh = sizeof(desc_phys) > 4 ? (desc_phys >> 32) & 0xFFFFFFFF : 0;
        pitems[items-1].dwHostAddrLow = desc_phys & 0xFFFFFFFF;
        pitems[items-1].dwTag = 0;
        pitems[items-1].dwFlagsAndLen =
            (DMA_DESC_LINK | (DMA_DESC_LEN_MASK & items));
    }

    return 0;
}

int xi_pcie_dma_desc_chain_set_item(
        struct pcie_dma_desc_chain *pobj,
        u32 item,
        bool irq_enable,
        bool flush,
        bool tag_valid,
        u32  tag,
        u32 host_addr_high,
        u32 host_addr_low,
        size_t length
        )
{
    PCIE_DMA_DESCRIPTOR * pitems =
            (PCIE_DMA_DESCRIPTOR *)(os_contig_dma_get_virt(pobj->addr_desc));

    if (item >= pobj->max_items)
        return -1;

	pitems[item].dwHostAddrHigh = host_addr_high;
	pitems[item].dwHostAddrLow  = host_addr_low;
	pitems[item].dwTag = tag;
	pitems[item].dwFlagsAndLen =
		(tag_valid ? DMA_DESC_TAG_VALID : 0)
		| (irq_enable ? DMA_DESC_IRQ_ENABLE : 0)
		| (flush ? DMA_DESC_FLUSH : 0)
		| (length & DMA_DESC_LEN_MASK);

    return 0;
}

static void _set_image_endtag_end_irq(
        struct pcie_dma_desc_chain *pobj,
        u32 item
        )
{
    PCIE_DMA_DESCRIPTOR * plastitem;
    xi_pcie_dma_desc_chain_set_num_items(pobj, item, FALSE);

    plastitem =
            (PCIE_DMA_DESCRIPTOR *)(os_contig_dma_get_virt(pobj->addr_desc)) + (item - 1);
    plastitem->dwFlagsAndLen |= (DMA_DESC_IRQ_ENABLE | DMA_DESC_TAG_VALID);
    plastitem->dwTag |= 0x80000000; // Frame end flag
}

/* chain build */
static bool _sg_transfer_video_line(
        struct pcie_dma_desc_chain *chain,
        u32 *item,
        mw_scatterlist_t **sgbuf,
        mw_scatterlist_t *sgone,
        size_t line,
        size_t padding,
        bool tag_valid,
        u32 tag,
        bool irq_enable,
        mw_scatterlist_t *sglast
        );
static unsigned long _sg_build_desc_chain_packed(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        int bpp,
        u32 stride,
        BOOLEAN bottom_up,
        LONG notify,
        const RECT *xfer_rect
        );
static unsigned long _sg_build_desc_chain_v210(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG notify,
        const RECT *xfer_rect
        );
static unsigned long _sg_build_desc_chain_i420(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG notify,
        const RECT *xfer_rect
        );
static unsigned long _sg_build_desc_chain_nv12(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG notify,
        const RECT *xfer_rect
        );
static unsigned long _sg_build_desc_chain_i422(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        );
static unsigned long _sg_build_desc_chain_nv16(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        );
static unsigned long _sg_build_desc_chain_p010(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        );
static unsigned long _sg_build_desc_chain_p210(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        );

static bool _sgbuf_move_forward(
        mw_scatterlist_t **sgbuf,
        mw_scatterlist_t *sgone,
        size_t offset,
        mw_scatterlist_t *sglast
        )
{
    while (offset > 0) {
        if (offset <= mw_sg_dma_len(sgone)) {
            mw_sg_dma_len(sgone) -= offset;
            mw_sg_dma_address(sgone) += offset;
            return true;
        } else {
            if (*sgbuf == sglast)
                return false;

            offset -= mw_sg_dma_len(sgone);
            *sgbuf = mw_sg_next(*sgbuf);

            os_memcpy(sgone, *sgbuf, sizeof(*sgone));
        }
    }

    return true;
}

static bool _sgbuf_move_backward(
        mw_scatterlist_t **sgbuf,
        mw_scatterlist_t *sgone,
        u32 offset,
        mw_scatterlist_t *sgfirst
        )
{
    while (offset > 0) {
        size_t one_offset = mw_sg_dma_address(sgone) - mw_sg_dma_address(*sgbuf);
        if (offset <= one_offset) {
            mw_sg_dma_len(sgone) += offset;
            mw_sg_dma_address(sgone) -= offset;
            return true;
        } else {
            if (*sgbuf == sgfirst)
                return false;

            offset -= one_offset;
            /* note the sg element should not be part of a chained scatterlist */
            (*sgbuf)--;
            mw_sg_dma_len(sgone) = 0;
            mw_sg_dma_address(sgone) = mw_sg_dma_address(*sgbuf) + mw_sg_dma_len(*sgbuf);
        }
    }
    return true;
}

int xi_pcie_dma_desc_chain_build(
        struct pcie_dma_desc_chain *pobj,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        u32 fourcc,
        long cx,
        long cy,
        int bpp,
        u32 stride,
        BOOLEAN bottom_up,
        LONG notify,
        const RECT *xfer_rect
        )
{
    if (FOURCC_IsPacked(fourcc)) {
        if (fourcc == MWFOURCC_V210
            || fourcc == MWFOURCC_U210)
            _sg_build_desc_chain_v210(pobj, sgbuf, sgnum, cx, cy, stride, bottom_up, notify, xfer_rect);
        else
            _sg_build_desc_chain_packed(pobj, sgbuf, sgnum, cx, cy, bpp, stride, bottom_up, notify, xfer_rect);
    }
    else {
        switch (fourcc) {
            case MWFOURCC_NV12:
            case MWFOURCC_NV21:
                _sg_build_desc_chain_nv12(pobj, sgbuf, sgnum, cx, cy, stride, bottom_up, notify, xfer_rect);
                break;
            case MWFOURCC_NV16:
            case MWFOURCC_NV61:
                _sg_build_desc_chain_nv16(pobj, sgbuf, sgnum, cx, cy, stride, bottom_up, notify, xfer_rect);
                break;
            case MWFOURCC_I422:
            case MWFOURCC_YV16:
                _sg_build_desc_chain_i422(pobj, sgbuf, sgnum, cx, cy, stride, bottom_up, notify, xfer_rect);
                break;
            case MWFOURCC_P010:
                _sg_build_desc_chain_p010(pobj, sgbuf, sgnum, cx, cy, stride, bottom_up, notify, xfer_rect);
                break;
            case MWFOURCC_P210:
                _sg_build_desc_chain_p210(pobj, sgbuf, sgnum, cx, cy, stride, bottom_up, notify, xfer_rect);
                break;
            case MWFOURCC_I420:
            case MWFOURCC_IYUV:
            case MWFOURCC_YV12:
                _sg_build_desc_chain_i420(pobj, sgbuf, sgnum, cx, cy, stride, bottom_up, notify, xfer_rect);
                break;
            default:
                return OS_RETURN_INVAL;
        }
    }
    return 0;
}

static bool _sg_transfer_video_line(
        struct pcie_dma_desc_chain *chain,
        u32 *item,
        mw_scatterlist_t **sgbuf,
        mw_scatterlist_t *sgone,
        size_t line,
        size_t padding,
        bool tag_valid,
        u32 tag,
        bool irq_enable,
        mw_scatterlist_t *sglast
        )
{
    u32 max_items = xi_pcie_dma_desc_chain_get_max_num_items(chain);
    bool line_start = true;

    if (NULL == item)
        return false;

    while (line > 0 && *item < max_items) {
        size_t transfer;
        mw_physical_addr_t addr = mw_sg_dma_address(sgone);

        if (line <= mw_sg_dma_len(sgone)) {
            transfer = line;
            mw_sg_dma_len(sgone) -= transfer;
            mw_sg_dma_address(sgone) += transfer;
        } else {
            if (*sgbuf == sglast)
                return false;

            transfer = mw_sg_dma_len(sgone);
            *sgbuf = mw_sg_next(*sgbuf);
            os_memcpy(sgone, *sgbuf, sizeof(*sgone));
        }

        if (transfer > 0) {
            xi_pcie_dma_desc_chain_set_item(
                        chain,
                        *item,
                        irq_enable && (line == transfer),
                        line_start,
                        tag_valid && (line == transfer),
                        tag,
                        sizeof(addr) > 4 ? ((addr >> 32) & UINT_MAX) : 0,
                        addr & UINT_MAX,
                        transfer
                        );
            (*item)++;

            line_start = false;
            line -= transfer;
        }
    }

    if (line > 0)
        return false;

    return _sgbuf_move_forward(sgbuf, sgone, padding, sglast);
}

static unsigned long _sg_build_desc_chain_packed(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        int bpp,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        )
{
    u32 item = 0;
    u32 line;
    u32 padding;
    mw_scatterlist_t sgone;
    int y;
    u32 offset = 0;
    mw_scatterlist_t *sgfirst = sgbuf;
    mw_scatterlist_t *sglast = &sgbuf[sgnum-1];

    if (xfer_rect) {
        cx = xfer_rect->right - xfer_rect->left;
        cy = xfer_rect->bottom - xfer_rect->top;
        offset = stride * xfer_rect->top + (bpp * xfer_rect->left / 8);
    }

    if (bottom_up)
        offset += stride * (cy - 1);

    line = (u32)(cx * bpp / 8);
    padding = stride - line;

    os_memcpy(&sgone, sgbuf, sizeof(sgone));
    _sgbuf_move_forward(&sgbuf, &sgone, offset, sglast);

    if (bottom_up) {
        for (y = 0; y < cy; y++) {
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);

            mw_scatterlist_t *_sgbuf_line = sgbuf;
            mw_scatterlist_t _sgone_line;
            os_memcpy(&_sgone_line, &sgone, sizeof(_sgone_line));

            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, line,
                        padding, true, y+1, irq_enable, sglast))
                break;

            if (y == cy - 1)
                break;

            _sgbuf_move_backward(&sgbuf, &sgone, stride, sgfirst);
        }
    } else {
        for (y = 0; y < cy; y++) {
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);
            bool last_line = (y == (cy - 1));

            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf, &sgone, line,
                        last_line ? 0 : padding, true, y+1, irq_enable, sglast))
                break;
        }
    }

    if (item <= 0)
        return 0;

    _set_image_endtag_end_irq(chain, item);

    return item;
}

static unsigned long _sg_build_desc_chain_v210(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        )
{
    u32 item = 0;
    u32 offset = 0;
    int quad_pixel = 0;
    u32 line_size;
    u32 padding;
    int y;

    mw_scatterlist_t *_sgbuf;
    mw_scatterlist_t _sgone;
    mw_scatterlist_t *sgfirst = sgbuf;
    mw_scatterlist_t *sglast = &sgbuf[sgnum-1];

    if (xfer_rect) {
        cx = xfer_rect->right - xfer_rect->left;
        cy = xfer_rect->bottom - xfer_rect->top;
        offset = stride * xfer_rect->top;
    }

    if (bottom_up)
        offset += stride * (cy - 1);

    quad_pixel = ((cx + 3) / 4);

    line_size = (u32)(quad_pixel / 3 * 32 + (quad_pixel % 3) * 16);
    padding = stride - line_size;

    _sgbuf = sgbuf;
    os_memcpy(&_sgone, _sgbuf, sizeof(_sgone));
    _sgbuf_move_forward(&_sgbuf, &_sgone, offset, sglast);

    if (bottom_up) {
        for (y = 0; y < cy; y++) {
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);

            mw_scatterlist_t *_sgbuf_line;
            mw_scatterlist_t _sgone_line;

            _sgbuf_line = _sgbuf;
            os_memcpy(&_sgone_line, &_sgone, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, line_size,
                        0, true, y + 1, irq_enable, sglast))
                break;

            if (y == (cy - 1))
                break;

            _sgbuf_move_backward(&_sgbuf, &_sgone, stride, sgfirst);
        }
    }
    else {
        for (y = 0; y < cy; y++) {
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);
            bool is_last_line = (y == (cy - 1));

            if (!_sg_transfer_video_line(
                    chain, &item, &_sgbuf, &_sgone, line_size,
                    is_last_line ? 0 : padding,
                    true, y + 1, irq_enable, sglast))
                break;
        }
    }

    if (item <= 0)
        return 0;

    _set_image_endtag_end_irq(chain, item);

    return item;
}

static unsigned long _sg_build_desc_chain_i420(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        )
{
    u32 item = 0;
    size_t uv_line;
    size_t ypadding;
    size_t uvpadding;
    int y;
    u32 yoffset = 0;
    size_t uoffset = stride * cy;
    size_t voffset = stride * cy * 5 / 4;

    mw_scatterlist_t *sgbuf_y;
    mw_scatterlist_t sgone_y;
    mw_scatterlist_t *sgbuf_u;
    mw_scatterlist_t sgone_u;
    mw_scatterlist_t *sgbuf_v;
    mw_scatterlist_t sgone_v;

    mw_scatterlist_t *sgfirst = sgbuf;
    mw_scatterlist_t *sglast = &sgbuf[sgnum-1];

    if (xfer_rect) {
        cx = xfer_rect->right - xfer_rect->left;
        cy = xfer_rect->bottom - xfer_rect->top;
        yoffset += stride * xfer_rect->top + xfer_rect->left;
        uoffset += stride * xfer_rect->top / 4 + xfer_rect->left / 2;
        voffset += stride * xfer_rect->top / 4 + xfer_rect->left / 2;
    }

    if (bottom_up) {
        yoffset += stride * (cy - 1);
        uoffset += (stride / 2) * (cy / 2 - 1);
        voffset += (stride / 2) * (cy / 2 - 1);
    }

    uv_line = cx / 2;
    ypadding = stride - cx;
    uvpadding = ypadding / 2;

    sgbuf_y = sgbuf;
    os_memcpy(&sgone_y, sgbuf_y, sizeof(sgone_y));
    _sgbuf_move_forward(&sgbuf_y, &sgone_y, yoffset, sglast);

    sgbuf_u = sgbuf_y;
    os_memcpy(&sgone_u, &sgone_y, sizeof(sgone_u));
    _sgbuf_move_forward(&sgbuf_u, &sgone_u, uoffset - yoffset, sglast);

    sgbuf_v = sgbuf_u;
    os_memcpy(&sgone_v, &sgone_u, sizeof(sgone_v));
    _sgbuf_move_forward(&sgbuf_v, &sgone_v, voffset - uoffset, sglast);

    if (bottom_up) {
        for (y = 0; y < cy; y += 2) {
            u32 tag = y + 2;
            bool irq_enable = (cy_notify != 0) && (((y + 2) % cy_notify) == 0);

            mw_scatterlist_t *_sgbuf_line;
            mw_scatterlist_t _sgone_line;

            // Transfer two Y lines
            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx,
                        0, false, tag, false, sglast))
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);

            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx,
                        0, false, tag, false, sglast))
                break;

            // Transfer U line
            _sgbuf_line = sgbuf_u;
            os_memcpy(&_sgone_line, &sgone_u, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, uv_line,
                        0, false, tag, false, sglast))
                break;

            // Transfer V line
            _sgbuf_line = sgbuf_v;
            os_memcpy(&_sgone_line, &sgone_v, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, uv_line,
                        0, true, tag, irq_enable, sglast))
                break;

            if (y + 2 == cy)
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);
            _sgbuf_move_backward(&sgbuf_u, &sgone_u, stride / 2, sgfirst);
            _sgbuf_move_backward(&sgbuf_v, &sgone_v, stride / 2, sgfirst);
        }
    } else {
        for (y = 0; y < cy; y += 2) {
            u32 tag = y + 2;
            bool irq_enable = (cy_notify != 0) && (((y + 2) % cy_notify) == 0);
            bool last_line = ((y / 2) == ((cy / 2) - 1));

            // Transfer two Y lines
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx,
                        ypadding, false, tag, false, sglast))
                break;
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx,
                        last_line ? 0 : ypadding, false, tag, false, sglast))
                break;

            // Transfer U line
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_u, &sgone_u, uv_line,
                        last_line ? 0 : uvpadding, false, tag, false, sglast))
                break;

            // Transfer V line
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_v, &sgone_v, uv_line,
                        last_line ? 0 : uvpadding, true, tag, irq_enable, sglast))
                break;
        }
    }

    if (item <= 0)
        return 0;

    _set_image_endtag_end_irq(chain, item);

    return item;
}

static unsigned long _sg_build_desc_chain_nv12(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        )
{
    u32 item = 0;
    size_t padding;
    int y;
    u32 yoffset = 0;
    size_t uvoffset = stride * cy;

    mw_scatterlist_t *sgbuf_y;
    mw_scatterlist_t sgone_y;
    mw_scatterlist_t *sgbuf_uv;
    mw_scatterlist_t sgone_uv;

    mw_scatterlist_t *sgfirst = sgbuf;
    mw_scatterlist_t *sglast = &sgbuf[sgnum-1];

    if (xfer_rect) {
        cx = xfer_rect->right - xfer_rect->left;
        cy = xfer_rect->bottom - xfer_rect->top;
        yoffset += stride * xfer_rect->top + xfer_rect->left;
        uvoffset += stride * xfer_rect->top / 2 + xfer_rect->left;
    }

    if (bottom_up) {
        yoffset += stride * (cy - 1);
        uvoffset += stride * (cy / 2 - 1);
    }

    padding = stride - cx;

    sgbuf_y = sgbuf;
    os_memcpy(&sgone_y, sgbuf_y, sizeof(sgone_y));
    _sgbuf_move_forward(&sgbuf_y, &sgone_y, yoffset, sglast);

    sgbuf_uv = sgbuf_y;
    os_memcpy(&sgone_uv, &sgone_y, sizeof(sgone_uv));
    _sgbuf_move_forward(&sgbuf_uv, &sgone_uv, uvoffset - yoffset, sglast);

    if (bottom_up) {
        for (y = 0; y < cy; y += 2) {
            u32 tag = y + 2;
            bool irq_enable = (cy_notify != 0) && (((y + 2) % cy_notify) == 0);

            mw_scatterlist_t *_sgbuf_line;
            mw_scatterlist_t _sgone_line;

            // Transfer two Y lines
            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx,
                        0, false, tag, false, sglast))
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);

            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx,
                        0, false, tag, false, sglast))
                break;

            // Transfer UV line
            _sgbuf_line = sgbuf_uv;
            os_memcpy(&_sgone_line, &sgone_uv, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx,
                        0, true, tag, irq_enable, sglast))
                break;

            if (y + 2 == cy)
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);
            _sgbuf_move_backward(&sgbuf_uv, &sgone_uv, stride, sgfirst);
        }
    } else {

        for (y = 0; y < cy; y += 2) {
            u32 tag = y + 2;
            bool irq_enable = (cy_notify != 0) && (((y + 2) % cy_notify) == 0);
            bool last_line = ((y / 2) == ((cy / 2) - 1));

            // Transfer two Y lines
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx,
                        padding, false, tag, false, sglast))
                break;
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx,
                        last_line ? 0 : padding, false, tag, false, sglast))
                break;

            // Transfer UV line
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_uv, &sgone_uv, cx,
                        last_line ? 0 : padding, true, tag, irq_enable, sglast))
                break;
        }
    }

    if (item <= 0)
        return 0;

    _set_image_endtag_end_irq(chain, item);

    return item;
}

static unsigned long _sg_build_desc_chain_i422(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        )
{
    u32 item = 0;
    size_t uv_line;
    size_t ypadding;
    size_t uvpadding;
    int y;
    u32 yoffset = 0;
    size_t uoffset = stride * cy;
    size_t voffset = stride * cy * 3 / 2;

    mw_scatterlist_t *sgbuf_y;
    mw_scatterlist_t sgone_y;
    mw_scatterlist_t *sgbuf_u;
    mw_scatterlist_t sgone_u;
    mw_scatterlist_t *sgbuf_v;
    mw_scatterlist_t sgone_v;

    mw_scatterlist_t *sgfirst = sgbuf;
    mw_scatterlist_t *sglast = &sgbuf[sgnum-1];

    if (xfer_rect) {
        cx = xfer_rect->right - xfer_rect->left;
        cy = xfer_rect->bottom - xfer_rect->top;
        yoffset += stride * xfer_rect->top + xfer_rect->left;
        uoffset += stride * xfer_rect->top / 2 + xfer_rect->left / 2;
        voffset += stride * xfer_rect->top / 2 + xfer_rect->left / 2;
    }

    if (bottom_up) {
        yoffset += stride * (cy - 1);
        uoffset += (stride / 2) * (cy - 1);
        voffset += (stride / 2) * (cy - 1);
    }

    uv_line = cx / 2;
    ypadding = stride - cx;
    uvpadding = ypadding / 2;

    sgbuf_y = sgbuf;
    os_memcpy(&sgone_y, sgbuf_y, sizeof(sgone_y));
    _sgbuf_move_forward(&sgbuf_y, &sgone_y, yoffset, sglast);

    sgbuf_u = sgbuf_y;
    os_memcpy(&sgone_u, &sgone_y, sizeof(sgone_u));
    _sgbuf_move_forward(&sgbuf_u, &sgone_u, uoffset - yoffset, sglast);

    sgbuf_v = sgbuf_u;
    os_memcpy(&sgone_v, &sgone_u, sizeof(sgone_v));
    _sgbuf_move_forward(&sgbuf_v, &sgone_v, voffset - uoffset, sglast);

    if (bottom_up) {
        for (y = 0; y < cy; y += 1) {
            u32 tag = y + 1;
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);

            mw_scatterlist_t *_sgbuf_line;
            mw_scatterlist_t _sgone_line;

            // Transfer Y lines
            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx,
                        0, false, tag, false, sglast))
                break;

            // Transfer U line
            _sgbuf_line = sgbuf_u;
            os_memcpy(&_sgone_line, &sgone_u, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, uv_line,
                        0, false, tag, false, sglast))
                break;

            // Transfer V line
            _sgbuf_line = sgbuf_v;
            os_memcpy(&_sgone_line, &sgone_v, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, uv_line,
                        0, true, tag, irq_enable, sglast))
                break;

            if (y + 1 == cy)
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);
            _sgbuf_move_backward(&sgbuf_u, &sgone_u, stride / 2, sgfirst);
            _sgbuf_move_backward(&sgbuf_v, &sgone_v, stride / 2, sgfirst);
        }
    } else {
        for (y = 0; y < cy; y += 1) {
            u32 tag = y + 1;
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);
            bool last_line = (y == cy - 1);

            // Transfer Y lines
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx,
                        last_line ? 0 : ypadding, false, tag, false, sglast))
                break;

            // Transfer U line
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_u, &sgone_u, uv_line,
                        last_line ? 0 : uvpadding, false, tag, false, sglast))
                break;

            // Transfer V line
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_v, &sgone_v, uv_line,
                        last_line ? 0 : uvpadding, true, tag, irq_enable, sglast))
                break;
        }
    }

    if (item <= 0)
        return 0;

    _set_image_endtag_end_irq(chain, item);

    return item;
}

static unsigned long _sg_build_desc_chain_nv16(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        )
{
    u32 item = 0;
    size_t padding;
    int y;
    u32 yoffset = 0;
    size_t uvoffset = stride * cy;

    mw_scatterlist_t *sgbuf_y;
    mw_scatterlist_t sgone_y;
    mw_scatterlist_t *sgbuf_uv;
    mw_scatterlist_t sgone_uv;

    mw_scatterlist_t *sgfirst = sgbuf;
    mw_scatterlist_t *sglast = &sgbuf[sgnum-1];

    if (xfer_rect) {
        cx = xfer_rect->right - xfer_rect->left;
        cy = xfer_rect->bottom - xfer_rect->top;
        yoffset += stride * xfer_rect->top + xfer_rect->left;
        uvoffset += stride * xfer_rect->top + xfer_rect->left;
    }

    if (bottom_up) {
        yoffset += stride * (cy - 1);
        uvoffset += stride * (cy - 1);
    }

    padding = stride - cx;

    sgbuf_y = sgbuf;
    os_memcpy(&sgone_y, sgbuf_y, sizeof(sgone_y));
    _sgbuf_move_forward(&sgbuf_y, &sgone_y, yoffset, sglast);

    sgbuf_uv = sgbuf_y;
    os_memcpy(&sgone_uv, &sgone_y, sizeof(sgone_uv));
    _sgbuf_move_forward(&sgbuf_uv, &sgone_uv, uvoffset - yoffset, sglast);

    if (bottom_up) {
        for (y = 0; y < cy; y += 1) {
            u32 tag = y + 1;
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);

            mw_scatterlist_t *_sgbuf_line;
            mw_scatterlist_t _sgone_line;

            // Transfer Y lines
            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx,
                        0, false, tag, false, sglast))
                break;

            // Transfer UV line
            _sgbuf_line = sgbuf_uv;
            os_memcpy(&_sgone_line, &sgone_uv, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx,
                        0, true, tag, irq_enable, sglast))
                break;

            if (y + 1 == cy)
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);
            _sgbuf_move_backward(&sgbuf_uv, &sgone_uv, stride, sgfirst);
        }
    } else {

        for (y = 0; y < cy; y += 1) {
            u32 tag = y + 1;
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);
            bool last_line = (y == cy - 1);

            // Transfer two Y lines
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx,
                        last_line ? 0 : padding, false, tag, false, sglast))
                break;

            // Transfer UV line
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_uv, &sgone_uv, cx,
                        last_line ? 0 : padding, true, tag, irq_enable, sglast))
                break;
        }
    }

    if (item <= 0)
        return 0;

    _set_image_endtag_end_irq(chain, item);

    return item;
}

static unsigned long _sg_build_desc_chain_p010(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        )
{
    u32 item = 0;
    size_t padding;
    int y;
    u32 yoffset = 0;
    size_t uvoffset = stride * cy;

    mw_scatterlist_t *sgbuf_y;
    mw_scatterlist_t sgone_y;
    mw_scatterlist_t *sgbuf_uv;
    mw_scatterlist_t sgone_uv;

    mw_scatterlist_t *sgfirst = sgbuf;
    mw_scatterlist_t *sglast = &sgbuf[sgnum-1];

    if (xfer_rect) {
        cx = xfer_rect->right - xfer_rect->left;
        cy = xfer_rect->bottom - xfer_rect->top;
        yoffset += stride * xfer_rect->top + xfer_rect->left;
        uvoffset += stride * xfer_rect->top / 2 + xfer_rect->left;
    }

    if (bottom_up) {
        yoffset += stride * (cy - 1);
        uvoffset += stride * (cy / 2 - 1);
    }

    padding = stride - cx * 2;

    sgbuf_y = sgbuf;
    os_memcpy(&sgone_y, sgbuf_y, sizeof(sgone_y));
    _sgbuf_move_forward(&sgbuf_y, &sgone_y, yoffset, sglast);

    sgbuf_uv = sgbuf_y;
    os_memcpy(&sgone_uv, &sgone_y, sizeof(sgone_uv));
    _sgbuf_move_forward(&sgbuf_uv, &sgone_uv, uvoffset - yoffset, sglast);

    if (bottom_up) {
        for (y = 0; y < cy; y += 2) {
            u32 tag = y + 2;
            bool irq_enable = (cy_notify != 0) && (((y + 2) % cy_notify) == 0);

            mw_scatterlist_t *_sgbuf_line;
            mw_scatterlist_t _sgone_line;

            // Transfer two Y lines
            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx * 2,
                        0, false, tag, false, sglast))
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);

            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx * 2,
                        0, false, tag, false, sglast))
                break;

            // Transfer UV line
            _sgbuf_line = sgbuf_uv;
            os_memcpy(&_sgone_line, &sgone_uv, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx * 2,
                        0, true, tag, irq_enable, sglast))
                break;

            if (y + 2 == cy)
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);
            _sgbuf_move_backward(&sgbuf_uv, &sgone_uv, stride, sgfirst);
        }
    } else {

        for (y = 0; y < cy; y += 2) {
            u32 tag = y + 2;
            bool irq_enable = (cy_notify != 0) && (((y + 2) % cy_notify) == 0);
            bool last_line = ((y / 2) == ((cy / 2) - 1));

            // Transfer two Y lines
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx * 2,
                        padding, false, tag, false, sglast))
                break;
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx * 2,
                        last_line ? 0 : padding, false, tag, false, sglast))
                break;

            // Transfer UV line
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_uv, &sgone_uv, cx * 2,
                        last_line ? 0 : padding, true, tag, irq_enable, sglast))
                break;
        }
    }

    if (item <= 0)
        return 0;

    _set_image_endtag_end_irq(chain, item);

    return item;
}

static unsigned long _sg_build_desc_chain_p210(
        struct pcie_dma_desc_chain *chain,
        mw_scatterlist_t *sgbuf,
        u32 sgnum,
        long cx,
        long cy,
        u32 stride,
        BOOLEAN bottom_up,
        LONG cy_notify,
        const RECT *xfer_rect
        )
{
    u32 item = 0;
    size_t padding;
    int y;
    u32 yoffset = 0;
    size_t uvoffset = stride * cy;

    mw_scatterlist_t *sgbuf_y;
    mw_scatterlist_t sgone_y;
    mw_scatterlist_t *sgbuf_uv;
    mw_scatterlist_t sgone_uv;

    mw_scatterlist_t *sgfirst = sgbuf;
    mw_scatterlist_t *sglast = &sgbuf[sgnum-1];

    if (xfer_rect) {
        cx = xfer_rect->right - xfer_rect->left;
        cy = xfer_rect->bottom - xfer_rect->top;
        yoffset += stride * xfer_rect->top + xfer_rect->left;
        uvoffset += stride * xfer_rect->top + xfer_rect->left;
    }

    if (bottom_up) {
        yoffset += stride * (cy - 1);
        uvoffset += stride * (cy - 1);
    }

    padding = stride - cx * 2;

    sgbuf_y = sgbuf;
    os_memcpy(&sgone_y, sgbuf_y, sizeof(sgone_y));
    _sgbuf_move_forward(&sgbuf_y, &sgone_y, yoffset, sglast);

    sgbuf_uv = sgbuf_y;
    os_memcpy(&sgone_uv, &sgone_y, sizeof(sgone_uv));
    _sgbuf_move_forward(&sgbuf_uv, &sgone_uv, uvoffset - yoffset, sglast);

    if (bottom_up) {
        for (y = 0; y < cy; y += 1) {
            u32 tag = y + 1;
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);

            mw_scatterlist_t *_sgbuf_line;
            mw_scatterlist_t _sgone_line;

            // Transfer Y lines
            _sgbuf_line = sgbuf_y;
            os_memcpy(&_sgone_line, &sgone_y, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx * 2,
                        0, false, tag, false, sglast))
                break;

            // Transfer UV line
            _sgbuf_line = sgbuf_uv;
            os_memcpy(&_sgone_line, &sgone_uv, sizeof(_sgone_line));
            if (!_sg_transfer_video_line(
                        chain, &item, &_sgbuf_line, &_sgone_line, cx * 2,
                        0, true, tag, irq_enable, sglast))
                break;

            if (y + 1 == cy)
                break;

            _sgbuf_move_backward(&sgbuf_y, &sgone_y, stride, sgfirst);
            _sgbuf_move_backward(&sgbuf_uv, &sgone_uv, stride, sgfirst);
        }
    } else {

        for (y = 0; y < cy; y += 1) {
            u32 tag = y + 1;
            bool irq_enable = (cy_notify != 0) && (((y + 1) % cy_notify) == 0);
            bool last_line = (y == cy - 1);

            // Transfer two Y lines
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_y, &sgone_y, cx * 2,
                        last_line ? 0 : padding, false, tag, false, sglast))
                break;

            // Transfer UV line
            if (!_sg_transfer_video_line(
                        chain, &item, &sgbuf_uv, &sgone_uv, cx * 2,
                        last_line ? 0 : padding, true, tag, irq_enable, sglast))
                break;
        }
    }

    if (item <= 0)
        return 0;

    _set_image_endtag_end_irq(chain, item);

    return item;
}
