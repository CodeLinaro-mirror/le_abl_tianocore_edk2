/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __EFI_IPCC_H__
#define __EFI_IPCC_H__

typedef struct _EFI_IPCC_PROTOCOL EfiIPCCProtocol;

/*
 * External reference to the IPCC Protocol GUID.
 */
extern EFI_GUID gEfiIpccProtocolGuid;

typedef VOID    *IPCCHandle;
typedef UINT32  IPCCSignal;
typedef UINT32  IPCCErr;
typedef VOID    *IPCCCbFnData;

typedef enum
{
  IPCC_P_MPROC     = 0,
  IPCC_P_COMPUTEL0 = 1,
  IPCC_P_COMPUTEL1 = 2,
  IPCC_P_PCIEMSI   = 3,
  IPCC_P_TOTAL
} IPCCProtocol;

typedef enum
{
  IPCC_C_AOP   = 0,
  IPCC_C_TZ    = 1,
  IPCC_C_MPSS  = 2,
  IPCC_C_LPASS = 3,
  IPCC_C_SLPI  = 4,
  IPCC_C_SDC   = 5,
  IPCC_C_NSP0  = 6,
  IPCC_C_CDSP  = IPCC_C_NSP0,
  IPCC_C_NPU   = 7,
  IPCC_C_APPS  = 8,
  IPCC_C_GPU   = 9,
  IPCC_C_CVP   = 10,
  IPCC_C_CAM   = 11,
  IPCC_C_VPU   = 12,
  IPCC_C_PCIE0 = 13,
  IPCC_C_PCIE1 = 14,
  IPCC_C_PCIE2 = 15,
  IPCC_C_SPSS  = 16,
  IPCC_C_SMSS  = 17,
  IPCC_C_NSP1  = 18,
  IPCC_C_PCIE3 = 19,
  IPCC_C_PCIE4 = 20,
  IPCC_C_PCIE5 = 21,
  IPCC_C_PCIE6 = 22,
  IPCC_C_TMESS = 23,
  IPCC_C_WPSS  = 24,
  IPCC_C_DPU   = 25,
  IPCC_C_IPA   = 26,
  IPCC_C_SAIL0 = 27,
  IPCC_C_SAIL1 = 28,
  IPCC_C_SAIL2 = 29,
  IPCC_C_SAIL3 = 30,
  IPCC_C_GPDSP0 = 31,
  IPCC_C_GPDSP1 = 32,
  IPCC_C_APSS_NS1 = 33,
  IPCC_C_APSS_NS2 = 34,
  IPCC_C_APSS_NS3 = 35,
  IPCC_C_APSS_NS4 = 36,
  IPCC_C_TOTAL
} IPCCClient;

typedef VOID (*IPCCCbFn)
        (IPCCCbFnData Data, IPCCClient SenderID, IPCCSignal Signal);

typedef IPCCErr (*EFI_IPCC_ATTACH)(
  IPCCHandle   *PHandle,
  IPCCProtocol Protocol
);

typedef IPCCErr (*EFI_IPCC_DETACH)(
  IPCCHandle *PHandle
);

typedef IPCCErr (*EFI_IPCC_REGISTERINTERRUPT)(
  IPCCHandle       Handle,
  IPCCClient       SenderCID,
  IPCCSignal       SignalLow,
  IPCCSignal       SignalHigh,
  IPCCCbFn         FunctionCb,
  IPCCCbFnData     Data
);

typedef IPCCErr (*EFI_IPCC_DEREGISTERINTERRUPT)(
  IPCCHandle       Handle,
  IPCCClient       SenderCID,
  IPCCSignal       SignalLow,
  IPCCSignal       SignalHigh
);

typedef IPCCErr (*EFI_IPCC_TRIGGER)(
  IPCCHandle       Handle,
  IPCCClient       TargetCID,
  IPCCSignal       SignalLow,
  IPCCSignal       SignalHigh
);

/*===========================================================================
  PROTOCOL INTERFACE
===========================================================================*/

typedef struct _EFI_IPCC_PROTOCOL {
  UINT64                           Version;
  EFI_IPCC_ATTACH                  IpccAttach;
  EFI_IPCC_DETACH                  IpccDetach;
  EFI_IPCC_REGISTERINTERRUPT       IpccRegisterInterrupt;
  EFI_IPCC_DEREGISTERINTERRUPT     IpccDeregisterInterrupt;
  EFI_IPCC_TRIGGER                 IpccTrigger;
} EfiIPCCProtocol;

#endif /* __EFI_IPCC_H__ */
