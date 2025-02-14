//**************************************************************************\
//* This file is property of and copyright by the ALICE Project            *\
//* ALICE Experiment at CERN, All rights reserved.                         *\
//*                                                                        *\
//* Primary Authors: Matthias Richter <Matthias.Richter@ift.uib.no>        *\
//*                  for The ALICE HLT Project.                            *\
//*                                                                        *\
//* Permission to use, copy, modify and distribute this software and its   *\
//* documentation strictly for non-commercial purposes is hereby granted   *\
//* without fee, provided that the above copyright notice appears in all   *\
//* copies and that both the copyright notice and this permission notice   *\
//* appear in the supporting documentation. The authors make no claims     *\
//* about the suitability of this software for any purpose. It is          *\
//* provided "as is" without express or implied warranty.                  *\
//**************************************************************************

/// \file GPUParam.inc
/// \author David Rohr, Sergey Gorbunov

#ifndef GPUPARAM_INC
#define GPUPARAM_INC

#include "GPUParam.h"
#include "GPUTPCGMMergedTrackHit.h"

namespace GPUCA_NAMESPACE
{
namespace gpu
{

MEM_CLASS_PRE()
GPUdi() void MEM_LG(GPUParam)::Slice2Global(int iSlice, float x, float y, float z, float* X, float* Y, float* Z) const
{
  // conversion of coordinates sector->global
  *X = x * SliceParam[iSlice].CosAlpha - y * SliceParam[iSlice].SinAlpha;
  *Y = y * SliceParam[iSlice].CosAlpha + x * SliceParam[iSlice].SinAlpha;
  *Z = z;
}

MEM_CLASS_PRE()
GPUdi() void MEM_LG(GPUParam)::Global2Slice(int iSlice, float X, float Y, float Z, float* x, float* y, float* z) const
{
  // conversion of coordinates global->sector
  *x = X * SliceParam[iSlice].CosAlpha + Y * SliceParam[iSlice].SinAlpha;
  *y = Y * SliceParam[iSlice].CosAlpha - X * SliceParam[iSlice].SinAlpha;
  *z = Z;
}

MEM_CLASS_PRE()
GPUdi() float MEM_LG(GPUParam)::GetClusterRMS(int yz, int type, float z, float angle2) const
{
  //* recalculate the cluster RMS wih respect to the track slope

  MakeType(const float*) c = ParamRMS0[yz][type];
  float v = c[0] + c[1] * z + c[2] * angle2;
  v = CAMath::Abs(v);
  return v;
}

MEM_CLASS_PRE()
GPUdi() void MEM_LG(GPUParam)::GetClusterRMS2(int iRow, float z, float sinPhi, float DzDs, float& ErrY2, float& ErrZ2) const
{
  int rowType = tpcGeometry.GetROC(iRow);
  if (rowType > 2) {
    rowType = 2; // TODO: Add type 3
  }
  z = CAMath::Abs((250.f - 0.275f) - CAMath::Abs(z));
  float s2 = sinPhi * sinPhi;
  if (s2 > 0.95f * 0.95f) {
    s2 = 0.95f * 0.95f;
  }
  float sec2 = 1.f / (1.f - s2);
  float angleY2 = s2 * sec2;          // dy/dx
  float angleZ2 = DzDs * DzDs * sec2; // dz/dx

  ErrY2 = GetClusterRMS(0, rowType, z, angleY2);
  ErrZ2 = GetClusterRMS(1, rowType, z, angleZ2);
  ErrY2 *= ErrY2;
  ErrZ2 *= ErrZ2;
}

MEM_CLASS_PRE()
GPUdi() float MEM_LG(GPUParam)::GetClusterError2(int yz, int type, float z, float angle2) const
{
  //* recalculate the cluster error wih respect to the track slope

  MakeType(const float*) c = ParamS0Par[yz][type];
  float v = c[0] + c[1] * z + c[2] * angle2 + c[3] * z * z + c[4] * angle2 * angle2 + c[5] * z * angle2;
  v = CAMath::Abs(v);
  if (v < 0.0001f) {
    v = 0.0001f;
  }
  v *= yz ? rec.ClusterError2CorrectionZ : rec.ClusterError2CorrectionY;
  return v;
}

MEM_CLASS_PRE()
GPUdi() void MEM_LG(GPUParam)::GetClusterErrors2(int iRow, float z, float sinPhi, float DzDs, float& ErrY2, float& ErrZ2) const
{
  // Calibrated cluster error from OCDB for Y and Z
  int rowType = tpcGeometry.GetROC(iRow);
  if (rowType > 2) {
    rowType = 2; // TODO: Add type 3
  }
  z = CAMath::Abs((250.f - 0.275f) - CAMath::Abs(z));
  float s2 = sinPhi * sinPhi;
  if (s2 > 0.95f * 0.95f) {
    s2 = 0.95f * 0.95f;
  }
  float sec2 = 1.f / (1.f - s2);
  float angleY2 = s2 * sec2;          // dy/dx
  float angleZ2 = DzDs * DzDs * sec2; // dz/dx

  ErrY2 = GetClusterError2(0, rowType, z, angleY2);
  ErrZ2 = GetClusterError2(1, rowType, z, angleZ2);
}

MEM_CLASS_PRE()
GPUdi() void MEM_LG(GPUParam)::UpdateClusterError2ByState(short clusterState, float& ErrY2, float& ErrZ2) const
{
  if (clusterState & GPUTPCGMMergedTrackHit::flagEdge) {
    ErrY2 += 0.35f;
    ErrZ2 += 0.15f;
  }
  if (clusterState & GPUTPCGMMergedTrackHit::flagSingle) {
    ErrY2 += 0.2f;
    ErrZ2 += 0.2f;
  }
  if (clusterState & (GPUTPCGMMergedTrackHit::flagSplitPad | GPUTPCGMMergedTrackHit::flagShared | GPUTPCGMMergedTrackHit::flagSingle)) {
    ErrY2 += 0.03f;
    ErrY2 *= 3;
  }
  if (clusterState & (GPUTPCGMMergedTrackHit::flagSplitTime | GPUTPCGMMergedTrackHit::flagShared | GPUTPCGMMergedTrackHit::flagSingle)) {
    ErrZ2 += 0.03f;
    ErrZ2 *= 3;
  }
}

} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif
