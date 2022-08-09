// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#pragma once

#ifndef HOBOVR_COMPONENTS_H
#define HOBOVR_COMPONENTS_H

#include <openvr_driver.h>
#include "driverlog.h"

#include <cmath>
#include <memory>

#define _stricmp strcasecmp

#include "hobovr_defines.h"

namespace hobovr {
  enum ELensMathType {
    Mt_Invalid = 0,
    Mt_Default = 1
  };

  // ext display component keys
  static const char *const k_pch_ExtDisplay_Section = "hobovr_comp_extendedDisplay";
  static const char *const k_pch_ExtDisplay_WindowX_Int32 = "windowX";
  static const char *const k_pch_ExtDisplay_WindowY_Int32 = "windowY";
  static const char *const k_pch_ExtDisplay_WindowWidth_Int32 = "windowWidth";
  static const char *const k_pch_ExtDisplay_WindowHeight_Int32 = "windowHeight";
  static const char *const k_pch_ExtDisplay_RenderWidth_Int32 = "renderWidth";
  static const char *const k_pch_ExtDisplay_RenderHeight_Int32 = "renderHeight";
  static const char *const k_pch_ExtDisplay_EyeGapOffset_Int = "EyeGapOffsetPx";
  static const char *const k_pch_ExtDisplay_IsDisplayReal_Bool = "IsDisplayRealDisplay";
  static const char *const k_pch_ExtDisplay_IsDisplayOnDesktop_bool = "IsDisplayOnDesktop";

  // ext display component keys related to the ELensMathType::Mt_Default distortion type
  static const char *const k_pch_ExtDisplay_DistortionK1_Float = "DistortionK1";
  static const char *const k_pch_ExtDisplay_DistortionK2_Float = "DistortionK2";
  static const char *const k_pch_ExtDisplay_ZoomWidth_Float = "ZoomWidth";
  static const char *const k_pch_ExtDisplay_ZoomHeight_Float = "ZoomHeight";
  static const char *const k_pch_ExtDisplay_FOVL_Float = "FOVleft";
  static const char *const k_pch_ExtDisplay_FOVR_Float = "FOVright";
  static const char *const k_pch_ExtDisplay_FOVT_Float = "FOVtop";
  static const char *const k_pch_ExtDisplay_FOVB_Float = "FOVbottom";

  // compile time component settings
  static const bool HobovrExtDisplayComp_doLensStuff = true;
  static const short HobovrExtDisplayComp_lensDistortionType = ELensMathType::Mt_Default; // has to be one of hobovr::ELensMathType

  class HobovrExtendedDisplayComponent: public vr::IVRDisplayComponent {
  public:
    HobovrExtendedDisplayComponent(){

      m_nWindowX = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                              k_pch_ExtDisplay_WindowX_Int32);

      m_nWindowY = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                              k_pch_ExtDisplay_WindowY_Int32);

      m_nWindowWidth = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                  k_pch_ExtDisplay_WindowWidth_Int32);

      m_nWindowHeight = vr::VRSettings()->GetInt32(
          k_pch_ExtDisplay_Section, k_pch_ExtDisplay_WindowHeight_Int32);

      m_nRenderWidth = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                  k_pch_ExtDisplay_RenderWidth_Int32);

      m_nRenderHeight = vr::VRSettings()->GetInt32(
          k_pch_ExtDisplay_Section, k_pch_ExtDisplay_RenderHeight_Int32);

      if constexpr(HobovrExtDisplayComp_doLensStuff){
        if constexpr(HobovrExtDisplayComp_lensDistortionType == ELensMathType::Mt_Default){
          m_fDistortionK1 = vr::VRSettings()->GetFloat(
              k_pch_ExtDisplay_Section, k_pch_ExtDisplay_DistortionK1_Float);

          m_fDistortionK2 = vr::VRSettings()->GetFloat(
              k_pch_ExtDisplay_Section, k_pch_ExtDisplay_DistortionK2_Float);

          m_fZoomWidth = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section,
                                                    k_pch_ExtDisplay_ZoomWidth_Float);

          m_fZoomHeight = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section,
                                                     k_pch_ExtDisplay_ZoomHeight_Float);
        }
      }

      m_fFovL = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section, k_pch_ExtDisplay_FOVL_Float);
      m_fFovR = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section, k_pch_ExtDisplay_FOVR_Float);
      m_fFovT = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section, k_pch_ExtDisplay_FOVT_Float);
      m_fFovB = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section, k_pch_ExtDisplay_FOVB_Float);

      m_iEyeGapOff = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                 k_pch_ExtDisplay_EyeGapOffset_Int);

      m_bIsDisplayReal = vr::VRSettings()->GetBool(k_pch_ExtDisplay_Section,
                                                 k_pch_ExtDisplay_IsDisplayReal_Bool);

      m_bIsDisplayOnDesktop = vr::VRSettings()->GetBool(k_pch_ExtDisplay_Section,
                                                 k_pch_ExtDisplay_IsDisplayOnDesktop_bool);

      DriverLog("Ext_display: component created\n");
      DriverLog("Ext_display: lens distortion enable: %d", HobovrExtDisplayComp_doLensStuff);

      if constexpr(HobovrExtDisplayComp_doLensStuff){
        DriverLog("Ext_display: distortion math type: %d", HobovrExtDisplayComp_lensDistortionType);
        if constexpr(HobovrExtDisplayComp_lensDistortionType == ELensMathType::Mt_Default)
          DriverLog("Ext_display: distortion coefficient: k1=%f, k2=%f, zw=%f, zh=%f", m_fDistortionK1, m_fDistortionK2, m_fZoomWidth, m_fZoomHeight);
      }

      DriverLog("Ext_display: fov: %f %f %f %f", m_fFovL, m_fFovR, m_fFovT, m_fFovB);
      DriverLog("Ext_display: eye gap offset: %d", m_iEyeGapOff);
      DriverLog("Ext_display: is display real: %d", (int)m_bIsDisplayReal);
      DriverLog("Ext_display: is display on desktop: %d", (int)m_bIsDisplayOnDesktop);
      DriverLog("Ext_display: window bounds: %d %d %d %d", m_nWindowX, m_nWindowY, m_nWindowWidth, m_nWindowHeight);
      DriverLog("Ext_display: render target: %d %d", m_nRenderWidth, m_nRenderHeight);
      DriverLog("Ext_display: left eye viewport: %d %d %d %d", 0, 0, m_nWindowWidth/2, m_nWindowHeight);
      DriverLog("Ext_display: right eye viewport: %d %d %d %d", m_nWindowWidth/2 + m_iEyeGapOff, 0, m_nWindowWidth/2, m_nWindowHeight);

    }

    // if there was a section settings change, the device's base will call this method
    void ReloadSectionSettings() {
      if constexpr(HobovrExtDisplayComp_doLensStuff){
        if constexpr(HobovrExtDisplayComp_lensDistortionType == ELensMathType::Mt_Default){
          m_fDistortionK1 = vr::VRSettings()->GetFloat(
              k_pch_ExtDisplay_Section, k_pch_ExtDisplay_DistortionK1_Float);

          m_fDistortionK2 = vr::VRSettings()->GetFloat(
              k_pch_ExtDisplay_Section, k_pch_ExtDisplay_DistortionK2_Float);

          m_fZoomWidth = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section,
                                                    k_pch_ExtDisplay_ZoomWidth_Float);

          m_fZoomHeight = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section,
                                                     k_pch_ExtDisplay_ZoomHeight_Float);
        }
      }

      m_iEyeGapOff = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                 k_pch_ExtDisplay_EyeGapOffset_Int);
    }

    virtual void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth,
                                 uint32_t *pnHeight) {
      *pnX = m_nWindowX;
      *pnY = m_nWindowY;
      *pnWidth = m_nWindowWidth;
      *pnHeight = m_nWindowHeight;
    }

    virtual bool IsDisplayOnDesktop() { return m_bIsDisplayOnDesktop; }

    virtual bool IsDisplayRealDisplay() { return m_bIsDisplayReal; }

    virtual void GetRecommendedRenderTargetSize(uint32_t *pnWidth,
                                                uint32_t *pnHeight) {
      *pnWidth = m_nRenderWidth;
      *pnHeight = m_nRenderHeight;
    }

    virtual void GetEyeOutputViewport(vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY,
                                      uint32_t *pnWidth, uint32_t *pnHeight) {
      *pnY = 0;
      *pnWidth = m_nWindowWidth / 2;
      *pnHeight = m_nWindowHeight;

      if (eEye == vr::Eye_Left) {
        *pnX = 0;
      } else {
        *pnX = m_nWindowWidth / 2 + m_iEyeGapOff;
      }
    }

    virtual void GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight,
                                  float *pfTop, float *pfBottom) {
      *pfTop = -m_fFovT;
      *pfBottom = m_fFovB;
      if (eEye == vr::Eye_Left) {
        *pfLeft = -m_fFovL;
        *pfRight = m_fFovR;
      } else {
        *pfLeft = -m_fFovR;
        *pfRight = m_fFovL;
      }
    }

    virtual vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU,
                                                      float fV) {
      vr::DistortionCoordinates_t coordinates;

      (void)eEye; // because its not used and i cant remove this argument

      if constexpr(HobovrExtDisplayComp_doLensStuff) {
        if constexpr(HobovrExtDisplayComp_lensDistortionType == ELensMathType::Mt_Default) {
          // Distortion implementation from
          // https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc#L232

          // in here 0.5f is the distortion center
          double rr = sqrt((fU - 0.5f) * (fU - 0.5f) + (fV - 0.5f) * (fV - 0.5f));
          double r2 = rr * (1 + m_fDistortionK1 * (rr * rr) +
                     m_fDistortionK2 * (rr * rr * rr * rr));
          double theta = atan2(fU - 0.5f, fV - 0.5f);
          auto hX = float(sin(theta) * r2) * m_fZoomWidth;
          auto hY = float(cos(theta) * r2) * m_fZoomHeight;

          coordinates.rfBlue[0] = hX + 0.5f;
          coordinates.rfBlue[1] = hY + 0.5f;
          coordinates.rfGreen[0] = hX + 0.5f;
          coordinates.rfGreen[1] = hY + 0.5f;
          coordinates.rfRed[0] = hX + 0.5f;
          coordinates.rfRed[1] = hY + 0.5f;
        }
      } else {
        coordinates.rfBlue[0] = fU;
        coordinates.rfBlue[1] = fV;
        coordinates.rfGreen[0] = fU;
        coordinates.rfGreen[1] = fV;
        coordinates.rfRed[0] = fU;
        coordinates.rfRed[1] = fV;
      }

      return coordinates;
    }

    const char* GetComponentNameAndVersion() {return vr::IVRDisplayComponent_Version;}

  private:
    int32_t m_nWindowX;
    int32_t m_nWindowY;
    int32_t m_nWindowWidth;
    int32_t m_nWindowHeight;
    int32_t m_nRenderWidth;
    int32_t m_nRenderHeight;
    int32_t m_iEyeGapOff;

    bool m_bIsDisplayReal;
    bool m_bIsDisplayOnDesktop;

    float m_fDistortionK1;
    float m_fDistortionK2;
    float m_fZoomWidth;
    float m_fZoomHeight;
    float m_fFovL;
    float m_fFovR;
    float m_fFovT;
    float m_fFovB;
  };

  // this is a dummy class meant to expand the component handling system, DO NOT USE THIS!
  class HobovrDriverDirectModeComponent {
  public:
    HobovrDriverDirectModeComponent() {}
    virtual void ReloadSectionSettings() {} // this is here for compatibility reasons
  };

  // this is a dummy class meant to expand the component handling system, DO NOT USE THIS!
  class HobovrCameraComponent {
  public:
    HobovrCameraComponent() {}
    virtual void ReloadSectionSettings() {} // this is here for compatibility reasons
  };

  // this is a dummy class meant to expand the component handling system, DO NOT USE THIS!
  class HobovrVirtualDisplayComponent {
  public:
    HobovrVirtualDisplayComponent() {}
    virtual void ReloadSectionSettings() {} // this is here for compatibility reasons
  };

}

#endif // HOBOVR_COMPONENTS_H