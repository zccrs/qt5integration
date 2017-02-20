#include "dxcbwmsupport.h"
#include "dplatformintegration.h"
#include "utility.h"

#include "qxcbconnection.h"
#include "qxcbscreen.h"

DPP_BEGIN_NAMESPACE

class _DXcbWMSupport : public DXcbWMSupport {};

Q_GLOBAL_STATIC(_DXcbWMSupport, globalXWMS)

DXcbWMSupport::DXcbWMSupport()
{
    updateNetWMAtoms();
    updateRootWindowProperties();
}

void DXcbWMSupport::emitWMChanged()
{
    emit windowManagerChanged();
}

void DXcbWMSupport::updateNetWMAtoms()
{
    net_wm_atoms.clear();

    xcb_window_t root = DPlatformIntegration::xcbConnection()->primaryScreen()->root();
    int offset = 0;
    int remaining = 0;
    xcb_connection_t *xcb_connection = DPlatformIntegration::xcbConnection()->xcb_connection();

    do {
        xcb_get_property_cookie_t cookie = xcb_get_property(xcb_connection, false, root,
                                                            DPlatformIntegration::xcbConnection()->atom(QXcbAtom::_NET_SUPPORTED),
                                                            XCB_ATOM_ATOM, offset, 1024);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection, cookie, NULL);
        if (!reply)
            break;

        remaining = 0;

        if (reply->type == XCB_ATOM_ATOM && reply->format == 32) {
            int len = xcb_get_property_value_length(reply)/sizeof(xcb_atom_t);
            xcb_atom_t *atoms = (xcb_atom_t *)xcb_get_property_value(reply);
            int s = net_wm_atoms.size();
            net_wm_atoms.resize(s + len);
            memcpy(net_wm_atoms.data() + s, atoms, len*sizeof(xcb_atom_t));

            remaining = reply->bytes_after;
            offset += len;
        }

        free(reply);
    } while (remaining > 0);

    _net_wm_deepin_blur_region_rounded_atom = Utility::internAtom(QT_STRINGIFY(_NET_WM_DEEPIN_BLUR_REGION_ROUNDED));
    _kde_net_wm_blur_rehind_region_atom = Utility::internAtom(QT_STRINGIFY(_KDE_NET_WM_BLUR_BEHIND_REGION));
    m_isDeepinWM = isSupportedByWM(_net_wm_deepin_blur_region_rounded_atom);

    updateHasBlurWindow();
}

void DXcbWMSupport::updateRootWindowProperties()
{
    root_window_properties.clear();

    xcb_window_t root = DPlatformIntegration::xcbConnection()->primaryScreen()->root();
    xcb_connection_t *xcb_connection = DPlatformIntegration::xcbConnection()->xcb_connection();

    xcb_list_properties_cookie_t cookie = xcb_list_properties(xcb_connection, root);
    xcb_list_properties_reply_t *reply = xcb_list_properties_reply(xcb_connection, cookie, NULL);

    if (!reply)
        return;

    int len = xcb_list_properties_atoms_length(reply);
    xcb_atom_t *atoms = (xcb_atom_t *)xcb_list_properties_atoms(reply);
    root_window_properties.resize(len);
    memcpy(root_window_properties.data(), atoms, len * sizeof(xcb_atom_t));

    free(reply);

    updateHasBlurWindow();
}

void DXcbWMSupport::updateHasBlurWindow()
{
    bool hasBlurWindow(m_isDeepinWM || isContainsForRootWindow(_kde_net_wm_blur_rehind_region_atom));

    if (m_hasBlurWindow == hasBlurWindow)
        return;

    m_hasBlurWindow = hasBlurWindow;
    emit hasBlurWindowChanged(hasBlurWindow);
}

DXcbWMSupport *DXcbWMSupport::instance()
{
    return globalXWMS;
}

bool DXcbWMSupport::connectWindowManagerChangedSignal(QObject *object, std::function<void ()> slot)
{
    if (!object)
        return QObject::connect(globalXWMS, &DXcbWMSupport::windowManagerChanged, slot);

    return QObject::connect(globalXWMS, &DXcbWMSupport::windowManagerChanged, object, slot);
}

bool DXcbWMSupport::connectHasBlurWindowChanged(QObject *object, std::function<void ()> slot)
{
    if (!object)
        return QObject::connect(globalXWMS, &DXcbWMSupport::hasBlurWindowChanged, slot);

    return QObject::connect(globalXWMS, &DXcbWMSupport::hasBlurWindowChanged, object, slot);
}

bool DXcbWMSupport::isDeepinWM() const
{
    return m_isDeepinWM;
}

bool DXcbWMSupport::isSupportedByWM(xcb_atom_t atom) const
{
    return net_wm_atoms.contains(atom);
}

bool DXcbWMSupport::isContainsForRootWindow(xcb_atom_t atom) const
{
    return root_window_properties.contains(atom);
}

bool DXcbWMSupport::hasBlurWindow() const
{
    return m_hasBlurWindow;
}

DPP_END_NAMESPACE
