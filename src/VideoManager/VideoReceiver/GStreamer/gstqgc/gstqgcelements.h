#pragma once

#include <gst/gst.h>

/*
 * Compatibility for GStreamer < 1.20.
 * QGC 5.0 uses GST_ELEMENT_REGISTER_* macros introduced in GStreamer 1.20,
 * but Android GStreamer 1.18.6 does not provide them.
 */
#ifndef GST_ELEMENT_REGISTER_DECLARE
#define GST_ELEMENT_REGISTER_DECLARE(element) \
G_BEGIN_DECLS \
gboolean G_PASTE(gst_element_register_, element) (GstPlugin * plugin); \
G_END_DECLS
#endif

#ifndef GST_ELEMENT_REGISTER
#define GST_ELEMENT_REGISTER(element, plugin) \
G_PASTE(gst_element_register_, element) (plugin)
#endif

#ifndef GST_ELEMENT_REGISTER_DEFINE_WITH_CODE
#define GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(e, e_n, r, t, _c_) \
G_BEGIN_DECLS \
gboolean G_PASTE(gst_element_register_, e) (GstPlugin * plugin) \
{ \
    _c_; \
    return gst_element_register(plugin, e_n, r, t); \
} \
G_END_DECLS
#endif

#ifndef GST_ELEMENT_REGISTER_DEFINE
#define GST_ELEMENT_REGISTER_DEFINE(e, e_n, r, t) \
GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(e, e_n, r, t, )
#endif

G_BEGIN_DECLS

extern GstDebugCategory *gst_qgc_debug;

void qgc_element_init(GstPlugin *plugin);

GST_ELEMENT_REGISTER_DECLARE(qgcvideosinkbin);

G_END_DECLS