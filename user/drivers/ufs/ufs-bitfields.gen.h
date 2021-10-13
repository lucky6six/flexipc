/*-
 * DO NOT MODIFY!
 * The file is generated from ufs.bitfields by the remap2h.py script.
 * Please update the source file and run the script again.
 */

#define HCS_PWR_OK     0 /* The request was accepted. */
#define HCS_PWR_LOCAL  1 /* The local request was successfully applied. */
#define HCS_PWR_REMOTE 2 /* The remote request was successfully applied. */
#define HCS_PWR_BUSY   3 /* The request was aborted due to concurrent requests. */
#define HCS_PWR_ERROR_CAP 4 /* The request was rejected because the requested configuration exceeded the Link’s capabilities. */
#define HCS_PWR_FATAL_ERROR 5 /* The request was aborted due to a communication problem. The Link may be inoperable. */
#define UECDL_EC_NAC_RECEIVED                 0
#define UECDL_EC_TCx_REPLAY_TIMER_EXPIRED     1
#define UECDL_EC_AFCx_REQUEST_TIMER_EXPIRED   2
#define UECDL_EC_FCx_PROTECTION_TIMER_EXPIRED 3
#define UECDL_EC_CRC_ERROR                    4
#define UECDL_EC_RX_BUFFER_OVERFLOW           5
#define UECDL_EC_MAX_FRAME_LENGTH_EXCEEDED    6
#define UECDL_EC_WRONG_SEQUENCE_NUMBER        7
#define UECDL_EC_AFC_FRAME_SYNTAX_ERROR       8
#define UECDL_EC_NAC_FRAME_SYNTAX_ERROR       9
#define UECDL_EC_EOF_SYNTAX_ERROR            10
#define UECDL_EC_FRAME_SYNTAX_ERROR          11
#define UECDL_EC_BAD_CTRL_SYMBOL_TYPE        12
#define UECDL_EC_PA_INIT_ERROR               13
#define UECDL_EC_PA_ERROR_IND_RECEIVED       14
#define UECN_EC_UNSUPPORTED_HEADER_TYPE   0
#define UECN_EC_BAD_DEVICEID_ENC          1
#define UECN_EC_LHDR_TRAP_PACKET_DROPPING 2
#define UECT_EC_UNSUPPORTED_HEADER_TYPE     0
#define UECT_EC_UNKNOWN_CPORTID             1
#define UECT_EC_NO_CONNECTION_RX            2
#define UECT_EC_CONTROLLED_SEGMENT_DROPPING 3
#define UECT_EC_BAD_TC                      4
#define UECT_EC_E2E_CREDIT_OVERFLOW         5
#define UECT_EC_SAFETY_VALVE_DROPPING       6
#define UICCMD_CMDOP_DME_GET      0x1
#define UICCMD_CMDOP_DME_SET      0x2
#define UICCMD_CMDOP_DME_PEER_GET 0x3
#define UICCMD_CMDOP_DME_PEER_SET 0x4
#define UICCMD_CMDOP_DME_POWERON  0x10
#define UICCMD_CMDOP_DME_POWEROFF 0x11
#define UICCMD_CMDOP_DME_ENABLE   0x12
#define UICCMD_CMDOP_DME_REST     0x14
#define UICCMD_CMDOP_DME_ENDPOINTRESET 0x15
#define UICCMD_CMDOP_DME_LINKSTARTUP   0x16
#define UICCMD_CMDOP_DME_HIBERNATE_ENTER 0x17
#define UICCMD_CMDOP_DME_HIBERNATE_EXIT  0x18
#define UICCMD_CMDOP_DME_TEST_MODE       0x1a
#define UICCMD_CRC_SUCCESS 0x00
#define UICCMD_CRC_INVALID_MIB_ATTRIBUTE 0x01
#define UICCMD_CRC_INVALID_MIB_ATTRIBUTE_VALUE 0x02
#define UICCMD_CRC_READ_ONLY_MIB_ATTRIBUTE 0x03
#define UICCMD_CRC_WRITE_ONLY_MIB_ATTRIBUTE 0x04
#define UICCMD_CRC_BAD_INDEX 0x05
#define UICCMD_CRC_LOCKED_MIB_ATTRIBUTE 0x06
#define UICCMD_CRC_BAD_TEST_FEATURE_INDEX 0x07
#define UICCMD_CRC_PEER_COMMUNICATION_FAILURE 0x08
#define UICCMD_CRC_BUSY 0x09
#define UICCMD_CRC_DME_FAILURE 0x0A
#define UICCMD_GEC_SUCCESS 0
#define UICCMD_GEC_FAILURE 1
#define TXFR_DD_NONDATA       0
#define TXFR_DD_SYSMEM_TO_DEV 1
#define TXFR_DD_DEV_TO_SYSMEM 2
#define UPIU_TXN_NOP_OUT     0
#define UPIU_TXN_COMMAND     1
#define UPIU_TXN_DATA_OUT    2
#define UPIU_TXN_TSKMGMT_REQ 4
#define UPIU_TXN_QUERY_REQ   0x16
#define UPIU_TXN_NOP_IN      0x20
#define UPIU_TXN_RESPONSE    0x21
#define UPIU_TXN_DATA_IN     0x22
#define UPIU_TXN_TSKMGMT_REP 0x24
#define UPIU_TXN_RTT         0x31 /* Ready to Response */
#define UPIU_TXN_QUERY_REP   0x36
#define UPIU_TXN_REJECT      0x3f
#define UPIU_TSKATTR_SIMPLE 0
#define UPIU_TSKATTR_ORDERED 1
#define UPIU_TSKATTR_HEADOFQ 2 /* Head of Queue */
#define UPIU_TSKATTR_ACA     3 /* Not Used */
#define UPIU_CP_NO_PRIORITY 0
#define UPIU_CP_HI_PRIORITY 1
#define UPIU_COMMAND_SET_SCSI 0
#define UPIU_COMMAND_SET_UFS  1
#define UPIU_REP_SUCCESS 0
#define UPIU_REP_FAILURE 1

// -------- bitfields regmap --------
//  Host Controller Capabiities

#define getp_regmap_CAP(regmap)   ((volatile u32 *)((char *)(regmap) + 0))
#define read_regmap_CAP(regmap)   (*getp_regmap_CAP(regmap))
#define write_regmap_CAP(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Crypto Support (CS)
#define read_regmap_CAP_CS(regmap) \
  (((read_regmap_CAP(regmap)) >> 28) & ((1ULL << 1) - 1))

#define write_regmap_CAP_CS(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 28)) | (((v) & ((1ULL << 1) - 1)) << 28); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC DME_TEST_MODE command supported (UICDMETMS)
#define read_regmap_CAP_UICDMETMS(regmap) \
  (((read_regmap_CAP(regmap)) >> 26) & ((1ULL << 1) - 1))

#define write_regmap_CAP_UICDMETMS(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 26)) | (((v) & ((1ULL << 1) - 1)) << 26); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Out of order data delivery supported (OODDS)
#define read_regmap_CAP_OODDS(regmap) \
  (((read_regmap_CAP(regmap)) >> 25) & ((1ULL << 1) - 1))

#define write_regmap_CAP_OODDS(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 25)) | (((v) & ((1ULL << 1) - 1)) << 25); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  64-bit addressing supported (64AS)
#define read_regmap_CAP_64AS(regmap) \
  (((read_regmap_CAP(regmap)) >> 24) & ((1ULL << 1) - 1))

#define write_regmap_CAP_64AS(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 24)) | (((v) & ((1ULL << 1) - 1)) << 24); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Auto-Hibernation Support (AUTOH8)
#define read_regmap_CAP_AUTOH8(regmap) \
  (((read_regmap_CAP(regmap)) >> 23) & ((1ULL << 1) - 1))

#define write_regmap_CAP_AUTOH8(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 23)) | (((v) & ((1ULL << 1) - 1)) << 23); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Number of UTP Task Management Request Slots (NUTMRS)
#define read_regmap_CAP_NUTMRS(regmap) \
  (((read_regmap_CAP(regmap)) >> 16) & ((1ULL << 3) - 1))

#define write_regmap_CAP_NUTMRS(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p) = ((*p) & ~(((1ULL << 3) - 1) << 16)) | (((v) & ((1ULL << 3) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Number of outstanding READY TO TRANSFER (RTT) requests supported (NORTT)
#define read_regmap_CAP_NORTT(regmap) \
  (((read_regmap_CAP(regmap)) >> 8) & ((1ULL << 8) - 1))

#define write_regmap_CAP_NORTT(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 8)) | (((v) & ((1ULL << 8) - 1)) << 8); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Number of UTP Transfer Request Slots (NUTRS)
#define read_regmap_CAP_NUTRS(regmap) \
  (((read_regmap_CAP(regmap)) >> 0) & ((1ULL << 5) - 1))

#define write_regmap_CAP_NUTRS(regmap, v) do { \
  typeof(getp_regmap_CAP(regmap)) p = getp_regmap_CAP(regmap); \
  (*p) = ((*p) & ~(((1ULL << 5) - 1) << 0)) | (((v) & ((1ULL << 5) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_regmap_word4(regmap)   ((volatile u32 *)((char *)(regmap) + 4))
#define read_regmap_word4(regmap)   (*getp_regmap_word4(regmap))
#define write_regmap_word4(regmap, v) do { \
  typeof(getp_regmap_word4(regmap)) p = getp_regmap_word4(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UFS Version

#define getp_regmap_VER(regmap)   ((volatile u32 *)((char *)(regmap) + 8))
#define read_regmap_VER(regmap)   (*getp_regmap_VER(regmap))
#define write_regmap_VER(regmap, v) do { \
  typeof(getp_regmap_VER(regmap)) p = getp_regmap_VER(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Major Version Number (MJR): Major version in BCD format
#define read_regmap_VER_MJR(regmap) \
  (((read_regmap_VER(regmap)) >> 8) & ((1ULL << 8) - 1))

#define write_regmap_VER_MJR(regmap, v) do { \
  typeof(getp_regmap_VER(regmap)) p = getp_regmap_VER(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 8)) | (((v) & ((1ULL << 8) - 1)) << 8); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Minor Version Number (MNR): Minor version in BCD format
#define read_regmap_VER_MNR(regmap) \
  (((read_regmap_VER(regmap)) >> 4) & ((1ULL << 4) - 1))

#define write_regmap_VER_MNR(regmap, v) do { \
  typeof(getp_regmap_VER(regmap)) p = getp_regmap_VER(regmap); \
  (*p) = ((*p) & ~(((1ULL << 4) - 1) << 4)) | (((v) & ((1ULL << 4) - 1)) << 4); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Version Suffix (VS): Version suffix in BCD format
#define read_regmap_VER_VS(regmap) \
  (((read_regmap_VER(regmap)) >> 0) & ((1ULL << 4) - 1))

#define write_regmap_VER_VS(regmap, v) do { \
  typeof(getp_regmap_VER(regmap)) p = getp_regmap_VER(regmap); \
  (*p) = ((*p) & ~(((1ULL << 4) - 1) << 0)) | (((v) & ((1ULL << 4) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_regmap_word12(regmap)   ((volatile u32 *)((char *)(regmap) + 12))
#define read_regmap_word12(regmap)   (*getp_regmap_word12(regmap))
#define write_regmap_word12(regmap, v) do { \
  typeof(getp_regmap_word12(regmap)) p = getp_regmap_word12(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host Controller Identification Descriptor – Device ID and Device Class

#define getp_regmap_HCPID(regmap)   ((volatile u32 *)((char *)(regmap) + 16))
#define read_regmap_HCPID(regmap)   (*getp_regmap_HCPID(regmap))
#define write_regmap_HCPID(regmap, v) do { \
  typeof(getp_regmap_HCPID(regmap)) p = getp_regmap_HCPID(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Product ID (PID)
#define read_regmap_HCPID_PID(regmap) \
  (((read_regmap_HCPID(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_HCPID_PID(regmap, v) do { \
  typeof(getp_regmap_HCPID(regmap)) p = getp_regmap_HCPID(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host Controller Identification Descriptor – Product ID and Manufacturer ID

#define getp_regmap_HCMID(regmap)   ((volatile u32 *)((char *)(regmap) + 20))
#define read_regmap_HCMID(regmap)   (*getp_regmap_HCMID(regmap))
#define write_regmap_HCMID(regmap, v) do { \
  typeof(getp_regmap_HCMID(regmap)) p = getp_regmap_HCMID(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Bank Index (BI)
#define read_regmap_HCMID_BI(regmap) \
  (((read_regmap_HCMID(regmap)) >> 8) & ((1ULL << 8) - 1))

#define write_regmap_HCMID_BI(regmap, v) do { \
  typeof(getp_regmap_HCMID(regmap)) p = getp_regmap_HCMID(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 8)) | (((v) & ((1ULL << 8) - 1)) << 8); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Manufacturer Identification Code (MIC)
#define read_regmap_HCMID_MIC(regmap) \
  (((read_regmap_HCMID(regmap)) >> 0) & ((1ULL << 8) - 1))

#define write_regmap_HCMID_MIC(regmap, v) do { \
  typeof(getp_regmap_HCMID(regmap)) p = getp_regmap_HCMID(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Auto-Hibernate Idle Timer

#define getp_regmap_AHIT(regmap)   ((volatile u32 *)((char *)(regmap) + 24))
#define read_regmap_AHIT(regmap)   (*getp_regmap_AHIT(regmap))
#define write_regmap_AHIT(regmap, v) do { \
  typeof(getp_regmap_AHIT(regmap)) p = getp_regmap_AHIT(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Timer scale(TS)
#define read_regmap_AHIT_TS(regmap) \
  (((read_regmap_AHIT(regmap)) >> 10) & ((1ULL << 3) - 1))

#define write_regmap_AHIT_TS(regmap, v) do { \
  typeof(getp_regmap_AHIT(regmap)) p = getp_regmap_AHIT(regmap); \
  (*p) = ((*p) & ~(((1ULL << 3) - 1) << 10)) | (((v) & ((1ULL << 3) - 1)) << 10); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Auto-Hibern8 Idle Timer Value (AH8ITV)
#define read_regmap_AHIT_AH8ITV(regmap) \
  (((read_regmap_AHIT(regmap)) >> 0) & ((1ULL << 10) - 1))

#define write_regmap_AHIT_AH8ITV(regmap, v) do { \
  typeof(getp_regmap_AHIT(regmap)) p = getp_regmap_AHIT(regmap); \
  (*p) = ((*p) & ~(((1ULL << 10) - 1) << 0)) | (((v) & ((1ULL << 10) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Reserved

#define getp_regmap_word28(regmap)   ((volatile u32 *)((char *)(regmap) + 28))
#define read_regmap_word28(regmap)   (*getp_regmap_word28(regmap))
#define write_regmap_word28(regmap, v) do { \
  typeof(getp_regmap_word28(regmap)) p = getp_regmap_word28(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Interrupt Status

#define getp_regmap_IS(regmap)   ((volatile u32 *)((char *)(regmap) + 32))
#define read_regmap_IS(regmap)   (*getp_regmap_IS(regmap))
#define write_regmap_IS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Crypto Engine Fatal Error Status (CEFES)
#define read_regmap_IS_CEFES(regmap) \
  (((read_regmap_IS(regmap)) >> 18) & ((1ULL << 1) - 1))

#define write_regmap_IS_CEFES(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 18)) | (((v) & ((1ULL << 1) - 1)) << 18); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  System Bus Fatal Error Status (SBFES)
#define read_regmap_IS_SBFES(regmap) \
  (((read_regmap_IS(regmap)) >> 17) & ((1ULL << 1) - 1))

#define write_regmap_IS_SBFES(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 17)) | (((v) & ((1ULL << 1) - 1)) << 17); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host Controller Fatal Error Status (HCFES)
#define read_regmap_IS_HCFES(regmap) \
  (((read_regmap_IS(regmap)) >> 16) & ((1ULL << 1) - 1))

#define write_regmap_IS_HCFES(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 16)) | (((v) & ((1ULL << 1) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Error Status (UTPES)
#define read_regmap_IS_UTPES(regmap) \
  (((read_regmap_IS(regmap)) >> 12) & ((1ULL << 1) - 1))

#define write_regmap_IS_UTPES(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 12)) | (((v) & ((1ULL << 1) - 1)) << 12); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Device Fatal Error Status (DFES)
#define read_regmap_IS_DFES(regmap) \
  (((read_regmap_IS(regmap)) >> 11) & ((1ULL << 1) - 1))

#define write_regmap_IS_DFES(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 11)) | (((v) & ((1ULL << 1) - 1)) << 11); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Command Completion Status (UCCS)
#define read_regmap_IS_UCCS(regmap) \
  (((read_regmap_IS(regmap)) >> 10) & ((1ULL << 1) - 1))

#define write_regmap_IS_UCCS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 10)) | (((v) & ((1ULL << 1) - 1)) << 10); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request Completion Status (UTMRCS)
#define read_regmap_IS_UTMRCS(regmap) \
  (((read_regmap_IS(regmap)) >> 9) & ((1ULL << 1) - 1))

#define write_regmap_IS_UTMRCS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 9)) | (((v) & ((1ULL << 1) - 1)) << 9); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Link Startup Status (ULSS)
#define read_regmap_IS_ULSS(regmap) \
  (((read_regmap_IS(regmap)) >> 8) & ((1ULL << 1) - 1))

#define write_regmap_IS_ULSS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 8)) | (((v) & ((1ULL << 1) - 1)) << 8); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Link Lost Status (ULLS)
#define read_regmap_IS_ULLS(regmap) \
  (((read_regmap_IS(regmap)) >> 7) & ((1ULL << 1) - 1))

#define write_regmap_IS_ULLS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 7)) | (((v) & ((1ULL << 1) - 1)) << 7); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Hibernate Enter Status (UHES)
#define read_regmap_IS_UHES(regmap) \
  (((read_regmap_IS(regmap)) >> 6) & ((1ULL << 1) - 1))

#define write_regmap_IS_UHES(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 6)) | (((v) & ((1ULL << 1) - 1)) << 6); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Hibernate Exit Status (UHXS)
#define read_regmap_IS_UHXS(regmap) \
  (((read_regmap_IS(regmap)) >> 5) & ((1ULL << 1) - 1))

#define write_regmap_IS_UHXS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 5)) | (((v) & ((1ULL << 1) - 1)) << 5); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Power Mode Status (UPMS)
#define read_regmap_IS_UPMS(regmap) \
  (((read_regmap_IS(regmap)) >> 4) & ((1ULL << 1) - 1))

#define write_regmap_IS_UPMS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 4)) | (((v) & ((1ULL << 1) - 1)) << 4); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Test Mode Status (UTMS)
#define read_regmap_IS_UTMS(regmap) \
  (((read_regmap_IS(regmap)) >> 3) & ((1ULL << 1) - 1))

#define write_regmap_IS_UTMS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 3)) | (((v) & ((1ULL << 1) - 1)) << 3); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Error (UE)
#define read_regmap_IS_UE(regmap) \
  (((read_regmap_IS(regmap)) >> 2) & ((1ULL << 1) - 1))

#define write_regmap_IS_UE(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 2)) | (((v) & ((1ULL << 1) - 1)) << 2); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC DME_ENDPOINTRESET Indication(UDEPRI)
#define read_regmap_IS_UDEPRI(regmap) \
  (((read_regmap_IS(regmap)) >> 1) & ((1ULL << 1) - 1))

#define write_regmap_IS_UDEPRI(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 1)) | (((v) & ((1ULL << 1) - 1)) << 1); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request Completion Status (UTRCS)
#define read_regmap_IS_UTRCS(regmap) \
  (((read_regmap_IS(regmap)) >> 0) & ((1ULL << 1) - 1))

#define write_regmap_IS_UTRCS(regmap, v) do { \
  typeof(getp_regmap_IS(regmap)) p = getp_regmap_IS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 0)) | (((v) & ((1ULL << 1) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Interrupt Enable

#define getp_regmap_IE(regmap)   ((volatile u32 *)((char *)(regmap) + 36))
#define read_regmap_IE(regmap)   (*getp_regmap_IE(regmap))
#define write_regmap_IE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Crypto Engine Fatal Error Enable (CEFEE)
#define read_regmap_IE_CEFEE(regmap) \
  (((read_regmap_IE(regmap)) >> 18) & ((1ULL << 1) - 1))

#define write_regmap_IE_CEFEE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 18)) | (((v) & ((1ULL << 1) - 1)) << 18); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  System Bus Fatal Error Enable (SBFEE)
#define read_regmap_IE_SBFEE(regmap) \
  (((read_regmap_IE(regmap)) >> 17) & ((1ULL << 1) - 1))

#define write_regmap_IE_SBFEE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 17)) | (((v) & ((1ULL << 1) - 1)) << 17); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host Controller Fatal Error Enable (HCFEE)
#define read_regmap_IE_HCFEE(regmap) \
  (((read_regmap_IE(regmap)) >> 16) & ((1ULL << 1) - 1))

#define write_regmap_IE_HCFEE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 16)) | (((v) & ((1ULL << 1) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Error Enable (UTPEE)
#define read_regmap_IE_UTPEE(regmap) \
  (((read_regmap_IE(regmap)) >> 12) & ((1ULL << 1) - 1))

#define write_regmap_IE_UTPEE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 12)) | (((v) & ((1ULL << 1) - 1)) << 12); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Device Fatal Error Enable (DFEE)
#define read_regmap_IE_DFEE(regmap) \
  (((read_regmap_IE(regmap)) >> 11) & ((1ULL << 1) - 1))

#define write_regmap_IE_DFEE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 11)) | (((v) & ((1ULL << 1) - 1)) << 11); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC COMMAND Completion Enable (UCCE)
#define read_regmap_IE_UCCE(regmap) \
  (((read_regmap_IE(regmap)) >> 10) & ((1ULL << 1) - 1))

#define write_regmap_IE_UCCE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 10)) | (((v) & ((1ULL << 1) - 1)) << 10); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request Completion Enable (UTMRCE)
#define read_regmap_IE_UTMRCE(regmap) \
  (((read_regmap_IE(regmap)) >> 9) & ((1ULL << 1) - 1))

#define write_regmap_IE_UTMRCE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 9)) | (((v) & ((1ULL << 1) - 1)) << 9); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Link Startup Status Enable (ULSSE)
#define read_regmap_IE_ULSSE(regmap) \
  (((read_regmap_IE(regmap)) >> 8) & ((1ULL << 1) - 1))

#define write_regmap_IE_ULSSE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 8)) | (((v) & ((1ULL << 1) - 1)) << 8); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Link Lost Status Enable (ULLSE)
#define read_regmap_IE_ULLSE(regmap) \
  (((read_regmap_IE(regmap)) >> 7) & ((1ULL << 1) - 1))

#define write_regmap_IE_ULLSE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 7)) | (((v) & ((1ULL << 1) - 1)) << 7); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Hibernate Enter Status Enable (UHESE)
#define read_regmap_IE_UHESE(regmap) \
  (((read_regmap_IE(regmap)) >> 6) & ((1ULL << 1) - 1))

#define write_regmap_IE_UHESE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 6)) | (((v) & ((1ULL << 1) - 1)) << 6); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Hibernate Exit Status Enable (UHXSE)
#define read_regmap_IE_UHXSE(regmap) \
  (((read_regmap_IE(regmap)) >> 5) & ((1ULL << 1) - 1))

#define write_regmap_IE_UHXSE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 5)) | (((v) & ((1ULL << 1) - 1)) << 5); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Power Mode Status Enable (UPMSE)
#define read_regmap_IE_UPMSE(regmap) \
  (((read_regmap_IE(regmap)) >> 4) & ((1ULL << 1) - 1))

#define write_regmap_IE_UPMSE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 4)) | (((v) & ((1ULL << 1) - 1)) << 4); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Test Mode Status Enable (UTMSE)
#define read_regmap_IE_UTMSE(regmap) \
  (((read_regmap_IE(regmap)) >> 3) & ((1ULL << 1) - 1))

#define write_regmap_IE_UTMSE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 3)) | (((v) & ((1ULL << 1) - 1)) << 3); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Error Enable (UEE)
#define read_regmap_IE_UEE(regmap) \
  (((read_regmap_IE(regmap)) >> 2) & ((1ULL << 1) - 1))

#define write_regmap_IE_UEE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 2)) | (((v) & ((1ULL << 1) - 1)) << 2); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC DME_ENDPOINTRESET (UDEPRIE)
#define read_regmap_IE_UDEPRIE(regmap) \
  (((read_regmap_IE(regmap)) >> 1) & ((1ULL << 1) - 1))

#define write_regmap_IE_UDEPRIE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 1)) | (((v) & ((1ULL << 1) - 1)) << 1); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request Completion Enable (UTRCE)
#define read_regmap_IE_UTRCE(regmap) \
  (((read_regmap_IE(regmap)) >> 0) & ((1ULL << 1) - 1))

#define write_regmap_IE_UTRCE(regmap, v) do { \
  typeof(getp_regmap_IE(regmap)) p = getp_regmap_IE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 0)) | (((v) & ((1ULL << 1) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_regmap_word40(regmap)   ((volatile u64 *)((char *)(regmap) + 40))
#define read_regmap_word40(regmap)   (*getp_regmap_word40(regmap))
#define write_regmap_word40(regmap, v) do { \
  typeof(getp_regmap_word40(regmap)) p = getp_regmap_word40(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host Controller Status

#define getp_regmap_HCS(regmap)   ((volatile u32 *)((char *)(regmap) + 48))
#define read_regmap_HCS(regmap)   (*getp_regmap_HCS(regmap))
#define write_regmap_HCS(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Target LUN of UTP error (TLUNUTPE)
#define read_regmap_HCS_TLUNUTPE(regmap) \
  (((read_regmap_HCS(regmap)) >> 24) & ((1ULL << 8) - 1))

#define write_regmap_HCS_TLUNUTPE(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 24)) | (((v) & ((1ULL << 8) - 1)) << 24); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Task Tag of UTP error (TTAGUTPE)
#define read_regmap_HCS_TTAGUTPE(regmap) \
  (((read_regmap_HCS(regmap)) >> 16) & ((1ULL << 8) - 1))

#define write_regmap_HCS_TTAGUTPE(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 16)) | (((v) & ((1ULL << 8) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Error Code (UTPEC)
#define read_regmap_HCS_UTPEC(regmap) \
  (((read_regmap_HCS(regmap)) >> 12) & ((1ULL << 4) - 1))

#define write_regmap_HCS_UTPEC(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 4) - 1) << 12)) | (((v) & ((1ULL << 4) - 1)) << 12); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Power Mode Change Request Status (UPMCRS)
#define read_regmap_HCS_UPMCRS(regmap) \
  (((read_regmap_HCS(regmap)) >> 8) & ((1ULL << 3) - 1))

#define write_regmap_HCS_UPMCRS(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 3) - 1) << 8)) | (((v) & ((1ULL << 3) - 1)) << 8); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC COMMAND Ready (UCRDY)
#define read_regmap_HCS_UCRDY(regmap) \
  (((read_regmap_HCS(regmap)) >> 3) & ((1ULL << 1) - 1))

#define write_regmap_HCS_UCRDY(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 3)) | (((v) & ((1ULL << 1) - 1)) << 3); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List Ready (UTMRLRDY)
#define read_regmap_HCS_UTMRLRDY(regmap) \
  (((read_regmap_HCS(regmap)) >> 2) & ((1ULL << 1) - 1))

#define write_regmap_HCS_UTMRLRDY(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 2)) | (((v) & ((1ULL << 1) - 1)) << 2); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Ready (UTRLRDY)
#define read_regmap_HCS_UTRLRDY(regmap) \
  (((read_regmap_HCS(regmap)) >> 1) & ((1ULL << 1) - 1))

#define write_regmap_HCS_UTRLRDY(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 1)) | (((v) & ((1ULL << 1) - 1)) << 1); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Device Present (DP)
#define read_regmap_HCS_DP(regmap) \
  (((read_regmap_HCS(regmap)) >> 0) & ((1ULL << 1) - 1))

#define write_regmap_HCS_DP(regmap, v) do { \
  typeof(getp_regmap_HCS(regmap)) p = getp_regmap_HCS(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 0)) | (((v) & ((1ULL << 1) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host Controller Enable

#define getp_regmap_HCE(regmap)   ((volatile u32 *)((char *)(regmap) + 52))
#define read_regmap_HCE(regmap)   (*getp_regmap_HCE(regmap))
#define write_regmap_HCE(regmap, v) do { \
  typeof(getp_regmap_HCE(regmap)) p = getp_regmap_HCE(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Crypto General Enable (CGE)
#define read_regmap_HCE_CGE(regmap) \
  (((read_regmap_HCE(regmap)) >> 1) & ((1ULL << 1) - 1))

#define write_regmap_HCE_CGE(regmap, v) do { \
  typeof(getp_regmap_HCE(regmap)) p = getp_regmap_HCE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 1)) | (((v) & ((1ULL << 1) - 1)) << 1); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host Controller Enable (HCE)
#define read_regmap_HCE_HCE(regmap) \
  (((read_regmap_HCE(regmap)) >> 0) & ((1ULL << 1) - 1))

#define write_regmap_HCE_HCE(regmap, v) do { \
  typeof(getp_regmap_HCE(regmap)) p = getp_regmap_HCE(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 0)) | (((v) & ((1ULL << 1) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host UIC Error Code PHY Adapter Layer

#define getp_regmap_UECPA(regmap)   ((volatile u32 *)((char *)(regmap) + 56))
#define read_regmap_UECPA(regmap)   (*getp_regmap_UECPA(regmap))
#define write_regmap_UECPA(regmap, v) do { \
  typeof(getp_regmap_UECPA(regmap)) p = getp_regmap_UECPA(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC PHY AdapterA Layer Error (ERR)
#define read_regmap_UECPA_ERR(regmap) \
  (((read_regmap_UECPA(regmap)) >> 31) & ((1ULL << 1) - 1))

#define write_regmap_UECPA_ERR(regmap, v) do { \
  typeof(getp_regmap_UECPA(regmap)) p = getp_regmap_UECPA(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 31)) | (((v) & ((1ULL << 1) - 1)) << 31); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC PHY Adapter Layer Error Code (EC)
#define read_regmap_UECPA_EC(regmap) \
  (((read_regmap_UECPA(regmap)) >> 0) & ((1ULL << 5) - 1))

#define write_regmap_UECPA_EC(regmap, v) do { \
  typeof(getp_regmap_UECPA(regmap)) p = getp_regmap_UECPA(regmap); \
  (*p) = ((*p) & ~(((1ULL << 5) - 1) << 0)) | (((v) & ((1ULL << 5) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host UIC Error Code Data Link Layer

#define getp_regmap_UECDL(regmap)   ((volatile u32 *)((char *)(regmap) + 60))
#define read_regmap_UECDL(regmap)   (*getp_regmap_UECDL(regmap))
#define write_regmap_UECDL(regmap, v) do { \
  typeof(getp_regmap_UECDL(regmap)) p = getp_regmap_UECDL(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Data Link Layer Error (ERR)
#define read_regmap_UECDL_ERR(regmap) \
  (((read_regmap_UECDL(regmap)) >> 31) & ((1ULL << 1) - 1))

#define write_regmap_UECDL_ERR(regmap, v) do { \
  typeof(getp_regmap_UECDL(regmap)) p = getp_regmap_UECDL(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 31)) | (((v) & ((1ULL << 1) - 1)) << 31); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Data Link Layer Error Code (EC)
#define read_regmap_UECDL_EC(regmap) \
  (((read_regmap_UECDL(regmap)) >> 0) & ((1ULL << 15) - 1))

#define write_regmap_UECDL_EC(regmap, v) do { \
  typeof(getp_regmap_UECDL(regmap)) p = getp_regmap_UECDL(regmap); \
  (*p) = ((*p) & ~(((1ULL << 15) - 1) << 0)) | (((v) & ((1ULL << 15) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host UIC Error Code Network Layer

#define getp_regmap_UECN(regmap)   ((volatile u32 *)((char *)(regmap) + 64))
#define read_regmap_UECN(regmap)   (*getp_regmap_UECN(regmap))
#define write_regmap_UECN(regmap, v) do { \
  typeof(getp_regmap_UECN(regmap)) p = getp_regmap_UECN(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Network Layer Error (ERR)
#define read_regmap_UECN_ERR(regmap) \
  (((read_regmap_UECN(regmap)) >> 31) & ((1ULL << 1) - 1))

#define write_regmap_UECN_ERR(regmap, v) do { \
  typeof(getp_regmap_UECN(regmap)) p = getp_regmap_UECN(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 31)) | (((v) & ((1ULL << 1) - 1)) << 31); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Network Layer Error Code (EC)
#define read_regmap_UECN_EC(regmap) \
  (((read_regmap_UECN(regmap)) >> 0) & ((1ULL << 3) - 1))

#define write_regmap_UECN_EC(regmap, v) do { \
  typeof(getp_regmap_UECN(regmap)) p = getp_regmap_UECN(regmap); \
  (*p) = ((*p) & ~(((1ULL << 3) - 1) << 0)) | (((v) & ((1ULL << 3) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host UIC Error Code Transport Layer

#define getp_regmap_UECT(regmap)   ((volatile u32 *)((char *)(regmap) + 68))
#define read_regmap_UECT(regmap)   (*getp_regmap_UECT(regmap))
#define write_regmap_UECT(regmap, v) do { \
  typeof(getp_regmap_UECT(regmap)) p = getp_regmap_UECT(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Transport Layer Error (ERR)
#define read_regmap_UECT_ERR(regmap) \
  (((read_regmap_UECT(regmap)) >> 31) & ((1ULL << 1) - 1))

#define write_regmap_UECT_ERR(regmap, v) do { \
  typeof(getp_regmap_UECT(regmap)) p = getp_regmap_UECT(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 31)) | (((v) & ((1ULL << 1) - 1)) << 31); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Transport Layer Error Code (EC)
#define read_regmap_UECT_EC(regmap) \
  (((read_regmap_UECT(regmap)) >> 0) & ((1ULL << 7) - 1))

#define write_regmap_UECT_EC(regmap, v) do { \
  typeof(getp_regmap_UECT(regmap)) p = getp_regmap_UECT(regmap); \
  (*p) = ((*p) & ~(((1ULL << 7) - 1) << 0)) | (((v) & ((1ULL << 7) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Host UIC Error Code DME

#define getp_regmap_UECDME(regmap)   ((volatile u32 *)((char *)(regmap) + 72))
#define read_regmap_UECDME(regmap)   (*getp_regmap_UECDME(regmap))
#define write_regmap_UECDME(regmap, v) do { \
  typeof(getp_regmap_UECDME(regmap)) p = getp_regmap_UECDME(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC DME Error (ERR)
#define read_regmap_UECDME_ERR(regmap) \
  (((read_regmap_UECDME(regmap)) >> 31) & ((1ULL << 1) - 1))

#define write_regmap_UECDME_ERR(regmap, v) do { \
  typeof(getp_regmap_UECDME(regmap)) p = getp_regmap_UECDME(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 31)) | (((v) & ((1ULL << 1) - 1)) << 31); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC DME Error Code (EC)
#define read_regmap_UECDME_EC(regmap) \
  (((read_regmap_UECDME(regmap)) >> 0) & ((1ULL << 1) - 1))

#define write_regmap_UECDME_EC(regmap, v) do { \
  typeof(getp_regmap_UECDME(regmap)) p = getp_regmap_UECDME(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 0)) | (((v) & ((1ULL << 1) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request Interrupt Aggregation Control Register

#define getp_regmap_UTRIACR(regmap)   ((volatile u32 *)((char *)(regmap) + 76))
#define read_regmap_UTRIACR(regmap)   (*getp_regmap_UTRIACR(regmap))
#define write_regmap_UTRIACR(regmap, v) do { \
  typeof(getp_regmap_UTRIACR(regmap)) p = getp_regmap_UTRIACR(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Interrupt Aggregation Enable/Disable (IAEN)
#define read_regmap_UTRIACR_IAEN(regmap) \
  (((read_regmap_UTRIACR(regmap)) >> 31) & ((1ULL << 1) - 1))

#define write_regmap_UTRIACR_IAEN(regmap, v) do { \
  typeof(getp_regmap_UTRIACR(regmap)) p = getp_regmap_UTRIACR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 31)) | (((v) & ((1ULL << 1) - 1)) << 31); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Interrupt aggregation parameter write enable (IAPWEN)
#define read_regmap_UTRIACR_IAPWEN(regmap) \
  (((read_regmap_UTRIACR(regmap)) >> 24) & ((1ULL << 1) - 1))

#define write_regmap_UTRIACR_IAPWEN(regmap, v) do { \
  typeof(getp_regmap_UTRIACR(regmap)) p = getp_regmap_UTRIACR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 24)) | (((v) & ((1ULL << 1) - 1)) << 24); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Interrupt aggregation status bit (IASB)
#define read_regmap_UTRIACR_IASB(regmap) \
  (((read_regmap_UTRIACR(regmap)) >> 20) & ((1ULL << 1) - 1))

#define write_regmap_UTRIACR_IASB(regmap, v) do { \
  typeof(getp_regmap_UTRIACR(regmap)) p = getp_regmap_UTRIACR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 20)) | (((v) & ((1ULL << 1) - 1)) << 20); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Counter and Timer Reset(CTR)
#define read_regmap_UTRIACR_CTR(regmap) \
  (((read_regmap_UTRIACR(regmap)) >> 16) & ((1ULL << 1) - 1))

#define write_regmap_UTRIACR_CTR(regmap, v) do { \
  typeof(getp_regmap_UTRIACR(regmap)) p = getp_regmap_UTRIACR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 16)) | (((v) & ((1ULL << 1) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Interrupt aggregation counter threshold (IACTH)
#define read_regmap_UTRIACR_IACTH(regmap) \
  (((read_regmap_UTRIACR(regmap)) >> 8) & ((1ULL << 5) - 1))

#define write_regmap_UTRIACR_IACTH(regmap, v) do { \
  typeof(getp_regmap_UTRIACR(regmap)) p = getp_regmap_UTRIACR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 5) - 1) << 8)) | (((v) & ((1ULL << 5) - 1)) << 8); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Interrupt aggregation timeout value (IATOVAL)
#define read_regmap_UTRIACR_IATOVAL(regmap) \
  (((read_regmap_UTRIACR(regmap)) >> 0) & ((1ULL << 8) - 1))

#define write_regmap_UTRIACR_IATOVAL(regmap, v) do { \
  typeof(getp_regmap_UTRIACR(regmap)) p = getp_regmap_UTRIACR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Base Address

#define getp_regmap_UTRLBA(regmap)   ((volatile u32 *)((char *)(regmap) + 80))
#define read_regmap_UTRLBA(regmap)   (*getp_regmap_UTRLBA(regmap))
#define write_regmap_UTRLBA(regmap, v) do { \
  typeof(getp_regmap_UTRLBA(regmap)) p = getp_regmap_UTRLBA(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Base Address (UTRLBA)
#define read_regmap_UTRLBA_UTRLBA(regmap) \
  (((read_regmap_UTRLBA(regmap)) >> 10) & ((1ULL << 22) - 1))

#define write_regmap_UTRLBA_UTRLBA(regmap, v) do { \
  typeof(getp_regmap_UTRLBA(regmap)) p = getp_regmap_UTRLBA(regmap); \
  (*p) = ((*p) & ~(((1ULL << 22) - 1) << 10)) | (((v) & ((1ULL << 22) - 1)) << 10); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Base Address Upper 32-Bits

#define getp_regmap_UTRLBAU(regmap)   ((volatile u32 *)((char *)(regmap) + 84))
#define read_regmap_UTRLBAU(regmap)   (*getp_regmap_UTRLBAU(regmap))
#define write_regmap_UTRLBAU(regmap, v) do { \
  typeof(getp_regmap_UTRLBAU(regmap)) p = getp_regmap_UTRLBAU(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Base Address Upper (UTRLBAU)
#define read_regmap_UTRLBAU_UTRLBAU(regmap) \
  (((read_regmap_UTRLBAU(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UTRLBAU_UTRLBAU(regmap, v) do { \
  typeof(getp_regmap_UTRLBAU(regmap)) p = getp_regmap_UTRLBAU(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Door Bell Register

#define getp_regmap_UTRLDBR(regmap)   ((volatile u32 *)((char *)(regmap) + 88))
#define read_regmap_UTRLDBR(regmap)   (*getp_regmap_UTRLDBR(regmap))
#define write_regmap_UTRLDBR(regmap, v) do { \
  typeof(getp_regmap_UTRLDBR(regmap)) p = getp_regmap_UTRLDBR(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Door bell Register (UTRLDBR)
#define read_regmap_UTRLDBR_UTRLDBR(regmap) \
  (((read_regmap_UTRLDBR(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UTRLDBR_UTRLDBR(regmap, v) do { \
  typeof(getp_regmap_UTRLDBR(regmap)) p = getp_regmap_UTRLDBR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List CLear Register

#define getp_regmap_UTRLCLR(regmap)   ((volatile u32 *)((char *)(regmap) + 92))
#define read_regmap_UTRLCLR(regmap)   (*getp_regmap_UTRLCLR(regmap))
#define write_regmap_UTRLCLR(regmap, v) do { \
  typeof(getp_regmap_UTRLCLR(regmap)) p = getp_regmap_UTRLCLR(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List CLear Register (UTRLCLR)
#define read_regmap_UTRLCLR_UTRLCLR(regmap) \
  (((read_regmap_UTRLCLR(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UTRLCLR_UTRLCLR(regmap, v) do { \
  typeof(getp_regmap_UTRLCLR(regmap)) p = getp_regmap_UTRLCLR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request Run-Stop Register

#define getp_regmap_UTRLRSR(regmap)   ((volatile u32 *)((char *)(regmap) + 96))
#define read_regmap_UTRLRSR(regmap)   (*getp_regmap_UTRLRSR(regmap))
#define write_regmap_UTRLRSR(regmap, v) do { \
  typeof(getp_regmap_UTRLRSR(regmap)) p = getp_regmap_UTRLRSR(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Run-Stop Register (UTRLRSR)
#define read_regmap_UTRLRSR_UTRLRSR(regmap) \
  (((read_regmap_UTRLRSR(regmap)) >> 0) & ((1ULL << 1) - 1))

#define write_regmap_UTRLRSR_UTRLRSR(regmap, v) do { \
  typeof(getp_regmap_UTRLRSR(regmap)) p = getp_regmap_UTRLRSR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 0)) | (((v) & ((1ULL << 1) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Completion Notification Register

#define getp_regmap_UTRLCNR(regmap)   ((volatile u32 *)((char *)(regmap) + 100))
#define read_regmap_UTRLCNR(regmap)   (*getp_regmap_UTRLCNR(regmap))
#define write_regmap_UTRLCNR(regmap, v) do { \
  typeof(getp_regmap_UTRLCNR(regmap)) p = getp_regmap_UTRLCNR(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Transfer Request List Completion Notification Register (UTRLCNR)
#define read_regmap_UTRLCNR_UTRLCNR(regmap) \
  (((read_regmap_UTRLCNR(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UTRLCNR_UTRLCNR(regmap, v) do { \
  typeof(getp_regmap_UTRLCNR(regmap)) p = getp_regmap_UTRLCNR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_regmap_word104(regmap)   ((volatile u64 *)((char *)(regmap) + 104))
#define read_regmap_word104(regmap)   (*getp_regmap_word104(regmap))
#define write_regmap_word104(regmap, v) do { \
  typeof(getp_regmap_word104(regmap)) p = getp_regmap_word104(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List Base Address

#define getp_regmap_UTMRLBA(regmap)   ((volatile u32 *)((char *)(regmap) + 112))
#define read_regmap_UTMRLBA(regmap)   (*getp_regmap_UTMRLBA(regmap))
#define write_regmap_UTMRLBA(regmap, v) do { \
  typeof(getp_regmap_UTMRLBA(regmap)) p = getp_regmap_UTMRLBA(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List Base Address (UTMRLBA)
#define read_regmap_UTMRLBA_UTMRLBA(regmap) \
  (((read_regmap_UTMRLBA(regmap)) >> 10) & ((1ULL << 22) - 1))

#define write_regmap_UTMRLBA_UTMRLBA(regmap, v) do { \
  typeof(getp_regmap_UTMRLBA(regmap)) p = getp_regmap_UTMRLBA(regmap); \
  (*p) = ((*p) & ~(((1ULL << 22) - 1) << 10)) | (((v) & ((1ULL << 22) - 1)) << 10); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List Base Address Upper 32-Bits

#define getp_regmap_UTMRLBAU(regmap)   ((volatile u32 *)((char *)(regmap) + 116))
#define read_regmap_UTMRLBAU(regmap)   (*getp_regmap_UTMRLBAU(regmap))
#define write_regmap_UTMRLBAU(regmap, v) do { \
  typeof(getp_regmap_UTMRLBAU(regmap)) p = getp_regmap_UTMRLBAU(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List Base Address (UTMRLBAU)
#define read_regmap_UTMRLBAU_UTMRLBAU(regmap) \
  (((read_regmap_UTMRLBAU(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UTMRLBAU_UTMRLBAU(regmap, v) do { \
  typeof(getp_regmap_UTMRLBAU(regmap)) p = getp_regmap_UTMRLBAU(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List Door Bell Register

#define getp_regmap_UTMRLDBR(regmap)   ((volatile u32 *)((char *)(regmap) + 120))
#define read_regmap_UTMRLDBR(regmap)   (*getp_regmap_UTMRLDBR(regmap))
#define write_regmap_UTMRLDBR(regmap, v) do { \
  typeof(getp_regmap_UTMRLDBR(regmap)) p = getp_regmap_UTMRLDBR(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List Door bell Register (UTMRLDBR)
#define read_regmap_UTMRLDBR_UTMRLDBR(regmap) \
  (((read_regmap_UTMRLDBR(regmap)) >> 0) & ((1ULL << 8) - 1))

#define write_regmap_UTMRLDBR_UTMRLDBR(regmap, v) do { \
  typeof(getp_regmap_UTMRLDBR(regmap)) p = getp_regmap_UTMRLDBR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List CLear Register

#define getp_regmap_UTMRLCLR(regmap)   ((volatile u32 *)((char *)(regmap) + 124))
#define read_regmap_UTMRLCLR(regmap)   (*getp_regmap_UTMRLCLR(regmap))
#define write_regmap_UTMRLCLR(regmap, v) do { \
  typeof(getp_regmap_UTMRLCLR(regmap)) p = getp_regmap_UTMRLCLR(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management List CLear Register (UTMRLCLR)
#define read_regmap_UTMRLCLR_UTMRLCLR(regmap) \
  (((read_regmap_UTMRLCLR(regmap)) >> 0) & ((1ULL << 8) - 1))

#define write_regmap_UTMRLCLR_UTMRLCLR(regmap, v) do { \
  typeof(getp_regmap_UTMRLCLR(regmap)) p = getp_regmap_UTMRLCLR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Run-Stop Register

#define getp_regmap_UTMRLRSR(regmap)   ((volatile u32 *)((char *)(regmap) + 128))
#define read_regmap_UTMRLRSR(regmap)   (*getp_regmap_UTMRLRSR(regmap))
#define write_regmap_UTMRLRSR(regmap, v) do { \
  typeof(getp_regmap_UTMRLRSR(regmap)) p = getp_regmap_UTMRLRSR(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Task Management Request List Run-Stop Register (UTMRLRSR)
#define read_regmap_UTMRLRSR_UTMRLRSR(regmap) \
  (((read_regmap_UTMRLRSR(regmap)) >> 0) & ((1ULL << 1) - 1))

#define write_regmap_UTMRLRSR_UTMRLRSR(regmap, v) do { \
  typeof(getp_regmap_UTMRLRSR(regmap)) p = getp_regmap_UTMRLRSR(regmap); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 0)) | (((v) & ((1ULL << 1) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_regmap_word132(regmap)   ((volatile u96 *)((char *)(regmap) + 132))
#define read_regmap_word132(regmap)   (*getp_regmap_word132(regmap))
#define write_regmap_word132(regmap, v) do { \
  typeof(getp_regmap_word132(regmap)) p = getp_regmap_word132(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Command Register

#define getp_regmap_UICCMD(regmap)   ((volatile u32 *)((char *)(regmap) + 144))
#define read_regmap_UICCMD(regmap)   (*getp_regmap_UICCMD(regmap))
#define write_regmap_UICCMD(regmap, v) do { \
  typeof(getp_regmap_UICCMD(regmap)) p = getp_regmap_UICCMD(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Command Opcode (CMDOP)
#define read_regmap_UICCMD_CMDOP(regmap) \
  (((read_regmap_UICCMD(regmap)) >> 0) & ((1ULL << 8) - 1))

#define write_regmap_UICCMD_CMDOP(regmap, v) do { \
  typeof(getp_regmap_UICCMD(regmap)) p = getp_regmap_UICCMD(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Command Argument 1

#define getp_regmap_UCMDARG1(regmap)   ((volatile u32 *)((char *)(regmap) + 148))
#define read_regmap_UCMDARG1(regmap)   (*getp_regmap_UCMDARG1(regmap))
#define write_regmap_UCMDARG1(regmap, v) do { \
  typeof(getp_regmap_UCMDARG1(regmap)) p = getp_regmap_UCMDARG1(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Argument 1 (ARG1)
#define read_regmap_UCMDARG1_ARG1(regmap) \
  (((read_regmap_UCMDARG1(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UCMDARG1_ARG1(regmap, v) do { \
  typeof(getp_regmap_UCMDARG1(regmap)) p = getp_regmap_UCMDARG1(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_regmap_UCMDARG1_GenSelectorIndex(regmap) \
  (((read_regmap_UCMDARG1(regmap)) >> 0) & ((1ULL << 16) - 1))

#define write_regmap_UCMDARG1_GenSelectorIndex(regmap, v) do { \
  typeof(getp_regmap_UCMDARG1(regmap)) p = getp_regmap_UCMDARG1(regmap); \
  (*p) = ((*p) & ~(((1ULL << 16) - 1) << 0)) | (((v) & ((1ULL << 16) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_regmap_UCMDARG1_MIBattribute(regmap) \
  (((read_regmap_UCMDARG1(regmap)) >> 16) & ((1ULL << 16) - 1))

#define write_regmap_UCMDARG1_MIBattribute(regmap, v) do { \
  typeof(getp_regmap_UCMDARG1(regmap)) p = getp_regmap_UCMDARG1(regmap); \
  (*p) = ((*p) & ~(((1ULL << 16) - 1) << 16)) | (((v) & ((1ULL << 16) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_regmap_UCMDARG1_ResetLevel(regmap) \
  (((read_regmap_UCMDARG1(regmap)) >> 0) & ((1ULL << 8) - 1))

#define write_regmap_UCMDARG1_ResetLevel(regmap, v) do { \
  typeof(getp_regmap_UCMDARG1(regmap)) p = getp_regmap_UCMDARG1(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Command Argument 2

#define getp_regmap_UCMDARG2(regmap)   ((volatile u32 *)((char *)(regmap) + 152))
#define read_regmap_UCMDARG2(regmap)   (*getp_regmap_UCMDARG2(regmap))
#define write_regmap_UCMDARG2(regmap, v) do { \
  typeof(getp_regmap_UCMDARG2(regmap)) p = getp_regmap_UCMDARG2(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Argument 2 (ARG2)
#define read_regmap_UCMDARG2_ARG2(regmap) \
  (((read_regmap_UCMDARG2(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UCMDARG2_ARG2(regmap, v) do { \
  typeof(getp_regmap_UCMDARG2(regmap)) p = getp_regmap_UCMDARG2(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_regmap_UCMDARG2_ConfigResultCode(regmap) \
  (((read_regmap_UCMDARG2(regmap)) >> 0) & ((1ULL << 8) - 1))

#define write_regmap_UCMDARG2_ConfigResultCode(regmap, v) do { \
  typeof(getp_regmap_UCMDARG2(regmap)) p = getp_regmap_UCMDARG2(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_regmap_UCMDARG2_GenericErrorCode(regmap) \
  (((read_regmap_UCMDARG2(regmap)) >> 0) & ((1ULL << 8) - 1))

#define write_regmap_UCMDARG2_GenericErrorCode(regmap, v) do { \
  typeof(getp_regmap_UCMDARG2(regmap)) p = getp_regmap_UCMDARG2(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_regmap_UCMDARG2_AttrSetType(regmap) \
  (((read_regmap_UCMDARG2(regmap)) >> 16) & ((1ULL << 8) - 1))

#define write_regmap_UCMDARG2_AttrSetType(regmap, v) do { \
  typeof(getp_regmap_UCMDARG2(regmap)) p = getp_regmap_UCMDARG2(regmap); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 16)) | (((v) & ((1ULL << 8) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UIC Command Argument 3

#define getp_regmap_UCMDARG3(regmap)   ((volatile u32 *)((char *)(regmap) + 156))
#define read_regmap_UCMDARG3(regmap)   (*getp_regmap_UCMDARG3(regmap))
#define write_regmap_UCMDARG3(regmap, v) do { \
  typeof(getp_regmap_UCMDARG3(regmap)) p = getp_regmap_UCMDARG3(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Argument 3 (ARG3)
#define read_regmap_UCMDARG3_ARG3(regmap) \
  (((read_regmap_UCMDARG3(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UCMDARG3_ARG3(regmap, v) do { \
  typeof(getp_regmap_UCMDARG3(regmap)) p = getp_regmap_UCMDARG3(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_regmap_UCMDARG3_MIBvalue_R(regmap) \
  (((read_regmap_UCMDARG3(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UCMDARG3_MIBvalue_R(regmap, v) do { \
  typeof(getp_regmap_UCMDARG3(regmap)) p = getp_regmap_UCMDARG3(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_regmap_UCMDARG3_MIBvalue_W(regmap) \
  (((read_regmap_UCMDARG3(regmap)) >> 0) & ((1ULL << 32) - 1))

#define write_regmap_UCMDARG3_MIBvalue_W(regmap, v) do { \
  typeof(getp_regmap_UCMDARG3(regmap)) p = getp_regmap_UCMDARG3(regmap); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_regmap_word160(regmap)   ((volatile u128 *)((char *)(regmap) + 160))
#define read_regmap_word160(regmap)   (*getp_regmap_word160(regmap))
#define write_regmap_word160(regmap, v) do { \
  typeof(getp_regmap_word160(regmap)) p = getp_regmap_word160(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Reserved for Unified Memory Extension

#define getp_regmap_word176(regmap)   ((volatile u128 *)((char *)(regmap) + 176))
#define read_regmap_word176(regmap)   (*getp_regmap_word176(regmap))
#define write_regmap_word176(regmap, v) do { \
  typeof(getp_regmap_word176(regmap)) p = getp_regmap_word176(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Vendor Specific Registers

#define getp_regmap_word192(regmap)   ((volatile u512 *)((char *)(regmap) + 192))
#define read_regmap_word192(regmap)   (*getp_regmap_word192(regmap))
#define write_regmap_word192(regmap, v) do { \
  typeof(getp_regmap_word192(regmap)) p = getp_regmap_word192(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Crypto Capability

#define getp_regmap_CCAP(regmap)   ((volatile u32 *)((char *)(regmap) + 256))
#define read_regmap_CCAP(regmap)   (*getp_regmap_CCAP(regmap))
#define write_regmap_CCAP(regmap, v) do { \
  typeof(getp_regmap_CCAP(regmap)) p = getp_regmap_CCAP(regmap); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
// -------- struct definitions of regmap --------
struct regmap {
	u32 CAP;
	u32 word4;
	u32 VER;
	u32 word12;
	u32 HCPID;
	u32 HCMID;
	u32 AHIT;
	u32 word28;
	u32 IS;
	u32 IE;
	u64 word40;
	u32 HCS;
	u32 HCE;
	u32 UECPA;
	u32 UECDL;
	u32 UECN;
	u32 UECT;
	u32 UECDME;
	u32 UTRIACR;
	u32 UTRLBA;
	u32 UTRLBAU;
	u32 UTRLDBR;
	u32 UTRLCLR;
	u32 UTRLRSR;
	u32 UTRLCNR;
	u64 word104;
	u32 UTMRLBA;
	u32 UTMRLBAU;
	u32 UTMRLDBR;
	u32 UTMRLCLR;
	u32 UTMRLRSR;
	u8 word132[12];
	u32 UICCMD;
	u32 UCMDARG1;
	u32 UCMDARG2;
	u32 UCMDARG3;
	u8 word160[16];
	u8 word176[16];
	u8 word192[64];
	u32 CCAP;

};
// -------- bitfields utp_txfr_reqdes --------

#define getp_utp_txfr_reqdes_word0(utp_txfr_reqdes)   ((volatile u32 *)((char *)(utp_txfr_reqdes) + 0))
#define read_utp_txfr_reqdes_word0(utp_txfr_reqdes)   (*getp_utp_txfr_reqdes_word0(utp_txfr_reqdes))
#define write_utp_txfr_reqdes_word0(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word0(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word0(utp_txfr_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Command Type (always 01h)
#define read_utp_txfr_reqdes_CT(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word0(utp_txfr_reqdes)) >> 28) & ((1ULL << 4) - 1))

#define write_utp_txfr_reqdes_CT(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word0(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word0(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 4) - 1) << 28)) | (((v) & ((1ULL << 4) - 1)) << 28); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Data Direction
#define read_utp_txfr_reqdes_DD(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word0(utp_txfr_reqdes)) >> 25) & ((1ULL << 2) - 1))

#define write_utp_txfr_reqdes_DD(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word0(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word0(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 2) - 1) << 25)) | (((v) & ((1ULL << 2) - 1)) << 25); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Iterrupt
#define read_utp_txfr_reqdes_I(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word0(utp_txfr_reqdes)) >> 24) & ((1ULL << 1) - 1))

#define write_utp_txfr_reqdes_I(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word0(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word0(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 24)) | (((v) & ((1ULL << 1) - 1)) << 24); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Crypto Enable
#define read_utp_txfr_reqdes_CE(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word0(utp_txfr_reqdes)) >> 23) & ((1ULL << 1) - 1))

#define write_utp_txfr_reqdes_CE(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word0(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word0(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 23)) | (((v) & ((1ULL << 1) - 1)) << 23); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Crypto Configuration Index
#define read_utp_txfr_reqdes_CCI(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word0(utp_txfr_reqdes)) >> 0) & ((1ULL << 8) - 1))

#define write_utp_txfr_reqdes_CCI(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word0(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word0(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_txfr_reqdes_word4(utp_txfr_reqdes)   ((volatile u32 *)((char *)(utp_txfr_reqdes) + 4))
#define read_utp_txfr_reqdes_word4(utp_txfr_reqdes)   (*getp_utp_txfr_reqdes_word4(utp_txfr_reqdes))
#define write_utp_txfr_reqdes_word4(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word4(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word4(utp_txfr_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Data Unit Number Lower 32 bits
#define read_utp_txfr_reqdes_DUNL(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word4(utp_txfr_reqdes)) >> 0) & ((1ULL << 32) - 1))

#define write_utp_txfr_reqdes_DUNL(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word4(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word4(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_txfr_reqdes_word8(utp_txfr_reqdes)   ((volatile u32 *)((char *)(utp_txfr_reqdes) + 8))
#define read_utp_txfr_reqdes_word8(utp_txfr_reqdes)   (*getp_utp_txfr_reqdes_word8(utp_txfr_reqdes))
#define write_utp_txfr_reqdes_word8(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word8(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word8(utp_txfr_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Overall Command Status
#define read_utp_txfr_reqdes_OCS(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word8(utp_txfr_reqdes)) >> 0) & ((1ULL << 8) - 1))

#define write_utp_txfr_reqdes_OCS(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word8(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word8(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 8) - 1) << 0)) | (((v) & ((1ULL << 8) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_txfr_reqdes_word12(utp_txfr_reqdes)   ((volatile u32 *)((char *)(utp_txfr_reqdes) + 12))
#define read_utp_txfr_reqdes_word12(utp_txfr_reqdes)   (*getp_utp_txfr_reqdes_word12(utp_txfr_reqdes))
#define write_utp_txfr_reqdes_word12(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word12(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word12(utp_txfr_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Data Unit Number Upper 32 bits
#define read_utp_txfr_reqdes_DUNU(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word12(utp_txfr_reqdes)) >> 0) & ((1ULL << 32) - 1))

#define write_utp_txfr_reqdes_DUNU(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word12(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word12(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_txfr_reqdes_word16(utp_txfr_reqdes)   ((volatile u32 *)((char *)(utp_txfr_reqdes) + 16))
#define read_utp_txfr_reqdes_word16(utp_txfr_reqdes)   (*getp_utp_txfr_reqdes_word16(utp_txfr_reqdes))
#define write_utp_txfr_reqdes_word16(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word16(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word16(utp_txfr_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Command Descriptor Base Address
#define read_utp_txfr_reqdes_UCDBA(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word16(utp_txfr_reqdes)) >> 7) & ((1ULL << 25) - 1))

#define write_utp_txfr_reqdes_UCDBA(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word16(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word16(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 25) - 1) << 7)) | (((v) & ((1ULL << 25) - 1)) << 7); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_txfr_reqdes_word20(utp_txfr_reqdes)   ((volatile u32 *)((char *)(utp_txfr_reqdes) + 20))
#define read_utp_txfr_reqdes_word20(utp_txfr_reqdes)   (*getp_utp_txfr_reqdes_word20(utp_txfr_reqdes))
#define write_utp_txfr_reqdes_word20(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word20(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word20(utp_txfr_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  UTP Command Descriptor Base Address Upper 32-bits
#define read_utp_txfr_reqdes_UCDBAU(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word20(utp_txfr_reqdes)) >> 0) & ((1ULL << 32) - 1))

#define write_utp_txfr_reqdes_UCDBAU(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word20(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word20(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_txfr_reqdes_word24(utp_txfr_reqdes)   ((volatile u32 *)((char *)(utp_txfr_reqdes) + 24))
#define read_utp_txfr_reqdes_word24(utp_txfr_reqdes)   (*getp_utp_txfr_reqdes_word24(utp_txfr_reqdes))
#define write_utp_txfr_reqdes_word24(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word24(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word24(utp_txfr_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Response UPIU Offset
#define read_utp_txfr_reqdes_RUO(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word24(utp_txfr_reqdes)) >> 16) & ((1ULL << 16) - 1))

#define write_utp_txfr_reqdes_RUO(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word24(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word24(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 16) - 1) << 16)) | (((v) & ((1ULL << 16) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Response UPIU Length
#define read_utp_txfr_reqdes_RUL(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word24(utp_txfr_reqdes)) >> 0) & ((1ULL << 16) - 1))

#define write_utp_txfr_reqdes_RUL(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word24(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word24(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 16) - 1) << 0)) | (((v) & ((1ULL << 16) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_txfr_reqdes_word28(utp_txfr_reqdes)   ((volatile u32 *)((char *)(utp_txfr_reqdes) + 28))
#define read_utp_txfr_reqdes_word28(utp_txfr_reqdes)   (*getp_utp_txfr_reqdes_word28(utp_txfr_reqdes))
#define write_utp_txfr_reqdes_word28(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word28(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word28(utp_txfr_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  PRDT Offset
#define read_utp_txfr_reqdes_PRDTO(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word28(utp_txfr_reqdes)) >> 16) & ((1ULL << 16) - 1))

#define write_utp_txfr_reqdes_PRDTO(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word28(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word28(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 16) - 1) << 16)) | (((v) & ((1ULL << 16) - 1)) << 16); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  PRDT Length
#define read_utp_txfr_reqdes_PRDTL(utp_txfr_reqdes) \
  (((read_utp_txfr_reqdes_word28(utp_txfr_reqdes)) >> 0) & ((1ULL << 16) - 1))

#define write_utp_txfr_reqdes_PRDTL(utp_txfr_reqdes, v) do { \
  typeof(getp_utp_txfr_reqdes_word28(utp_txfr_reqdes)) p = getp_utp_txfr_reqdes_word28(utp_txfr_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 16) - 1) << 0)) | (((v) & ((1ULL << 16) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
// -------- struct definitions of utp_txfr_reqdes --------
struct utp_txfr_reqdes {
	u32 word0;
	u32 word4;
	u32 word8;
	u32 word12;
	u32 word16;
	u32 word20;
	u32 word24;
	u32 word28;

};
// -------- bitfields phys_region_des --------

#define getp_phys_region_des_word0(phys_region_des)   ((volatile u32 *)((char *)(phys_region_des) + 0))
#define read_phys_region_des_word0(phys_region_des)   (*getp_phys_region_des_word0(phys_region_des))
#define write_phys_region_des_word0(phys_region_des, v) do { \
  typeof(getp_phys_region_des_word0(phys_region_des)) p = getp_phys_region_des_word0(phys_region_des); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Data Base Address
#define read_phys_region_des_DBA(phys_region_des) \
  (((read_phys_region_des_word0(phys_region_des)) >> 2) & ((1ULL << 30) - 1))

#define write_phys_region_des_DBA(phys_region_des, v) do { \
  typeof(getp_phys_region_des_word0(phys_region_des)) p = getp_phys_region_des_word0(phys_region_des); \
  (*p) = ((*p) & ~(((1ULL << 30) - 1) << 2)) | (((v) & ((1ULL << 30) - 1)) << 2); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_phys_region_des_word4(phys_region_des)   ((volatile u32 *)((char *)(phys_region_des) + 4))
#define read_phys_region_des_word4(phys_region_des)   (*getp_phys_region_des_word4(phys_region_des))
#define write_phys_region_des_word4(phys_region_des, v) do { \
  typeof(getp_phys_region_des_word4(phys_region_des)) p = getp_phys_region_des_word4(phys_region_des); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Data Base Adderss Upper 32-bits
#define read_phys_region_des_DBAU(phys_region_des) \
  (((read_phys_region_des_word4(phys_region_des)) >> 0) & ((1ULL << 32) - 1))

#define write_phys_region_des_DBAU(phys_region_des, v) do { \
  typeof(getp_phys_region_des_word4(phys_region_des)) p = getp_phys_region_des_word4(phys_region_des); \
  (*p) = ((*p) & ~(((1ULL << 32) - 1) << 0)) | (((v) & ((1ULL << 32) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_phys_region_des_word8(phys_region_des)   ((volatile u32 *)((char *)(phys_region_des) + 8))
#define read_phys_region_des_word8(phys_region_des)   (*getp_phys_region_des_word8(phys_region_des))
#define write_phys_region_des_word8(phys_region_des, v) do { \
  typeof(getp_phys_region_des_word8(phys_region_des)) p = getp_phys_region_des_word8(phys_region_des); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_phys_region_des_word12(phys_region_des)   ((volatile u32 *)((char *)(phys_region_des) + 12))
#define read_phys_region_des_word12(phys_region_des)   (*getp_phys_region_des_word12(phys_region_des))
#define write_phys_region_des_word12(phys_region_des, v) do { \
  typeof(getp_phys_region_des_word12(phys_region_des)) p = getp_phys_region_des_word12(phys_region_des); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Data Byte Count
#define read_phys_region_des_DBC(phys_region_des) \
  (((read_phys_region_des_word12(phys_region_des)) >> 0) & ((1ULL << 18) - 1))

#define write_phys_region_des_DBC(phys_region_des, v) do { \
  typeof(getp_phys_region_des_word12(phys_region_des)) p = getp_phys_region_des_word12(phys_region_des); \
  (*p) = ((*p) & ~(((1ULL << 18) - 1) << 0)) | (((v) & ((1ULL << 18) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Granularity, should be 0b11
#define read_phys_region_des_GRAN(phys_region_des) \
  (((read_phys_region_des_word12(phys_region_des)) >> 0) & ((1ULL << 2) - 1))

#define write_phys_region_des_GRAN(phys_region_des, v) do { \
  typeof(getp_phys_region_des_word12(phys_region_des)) p = getp_phys_region_des_word12(phys_region_des); \
  (*p) = ((*p) & ~(((1ULL << 2) - 1) << 0)) | (((v) & ((1ULL << 2) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
// -------- struct definitions of phys_region_des --------
struct phys_region_des {
	u32 word0;
	u32 word4;
	u32 word8;
	u32 word12;

};
// -------- bitfields utp_tskmgmt_reqdes --------

#define getp_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes)   ((volatile u32 *)((char *)(utp_tskmgmt_reqdes) + 0))
#define read_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes)   (*getp_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes))
#define write_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes, v) do { \
  typeof(getp_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes)) p = getp_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Interrupt
#define read_utp_tskmgmt_reqdes_I(utp_tskmgmt_reqdes) \
  (((read_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes)) >> 24) & ((1ULL << 1) - 1))

#define write_utp_tskmgmt_reqdes_I(utp_tskmgmt_reqdes, v) do { \
  typeof(getp_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes)) p = getp_utp_tskmgmt_reqdes_word0(utp_tskmgmt_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 24)) | (((v) & ((1ULL << 1) - 1)) << 24); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_tskmgmt_reqdes_word4(utp_tskmgmt_reqdes)   ((volatile u32 *)((char *)(utp_tskmgmt_reqdes) + 4))
#define read_utp_tskmgmt_reqdes_word4(utp_tskmgmt_reqdes)   (*getp_utp_tskmgmt_reqdes_word4(utp_tskmgmt_reqdes))
#define write_utp_tskmgmt_reqdes_word4(utp_tskmgmt_reqdes, v) do { \
  typeof(getp_utp_tskmgmt_reqdes_word4(utp_tskmgmt_reqdes)) p = getp_utp_tskmgmt_reqdes_word4(utp_tskmgmt_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes)   ((volatile u32 *)((char *)(utp_tskmgmt_reqdes) + 8))
#define read_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes)   (*getp_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes))
#define write_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes, v) do { \
  typeof(getp_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes)) p = getp_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Overall Command Status
#define read_utp_tskmgmt_reqdes_OCS(utp_tskmgmt_reqdes) \
  (((read_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes)) >> 0) & ((1ULL << 9) - 1))

#define write_utp_tskmgmt_reqdes_OCS(utp_tskmgmt_reqdes, v) do { \
  typeof(getp_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes)) p = getp_utp_tskmgmt_reqdes_word8(utp_tskmgmt_reqdes); \
  (*p) = ((*p) & ~(((1ULL << 9) - 1) << 0)) | (((v) & ((1ULL << 9) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_utp_tskmgmt_reqdes_word12(utp_tskmgmt_reqdes)   ((volatile u32 *)((char *)(utp_tskmgmt_reqdes) + 12))
#define read_utp_tskmgmt_reqdes_word12(utp_tskmgmt_reqdes)   (*getp_utp_tskmgmt_reqdes_word12(utp_tskmgmt_reqdes))
#define write_utp_tskmgmt_reqdes_word12(utp_tskmgmt_reqdes, v) do { \
  typeof(getp_utp_tskmgmt_reqdes_word12(utp_tskmgmt_reqdes)) p = getp_utp_tskmgmt_reqdes_word12(utp_tskmgmt_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Task Management Request UPIU

#define getp_utp_tskmgmt_reqdes_word16(utp_tskmgmt_reqdes)   ((volatile u256 *)((char *)(utp_tskmgmt_reqdes) + 16))
#define read_utp_tskmgmt_reqdes_word16(utp_tskmgmt_reqdes)   (*getp_utp_tskmgmt_reqdes_word16(utp_tskmgmt_reqdes))
#define write_utp_tskmgmt_reqdes_word16(utp_tskmgmt_reqdes, v) do { \
  typeof(getp_utp_tskmgmt_reqdes_word16(utp_tskmgmt_reqdes)) p = getp_utp_tskmgmt_reqdes_word16(utp_tskmgmt_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Task Management Response UPIU

#define getp_utp_tskmgmt_reqdes_word48(utp_tskmgmt_reqdes)   ((volatile u256 *)((char *)(utp_tskmgmt_reqdes) + 48))
#define read_utp_tskmgmt_reqdes_word48(utp_tskmgmt_reqdes)   (*getp_utp_tskmgmt_reqdes_word48(utp_tskmgmt_reqdes))
#define write_utp_tskmgmt_reqdes_word48(utp_tskmgmt_reqdes, v) do { \
  typeof(getp_utp_tskmgmt_reqdes_word48(utp_tskmgmt_reqdes)) p = getp_utp_tskmgmt_reqdes_word48(utp_tskmgmt_reqdes); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
// -------- struct definitions of utp_tskmgmt_reqdes --------
struct utp_tskmgmt_reqdes {
	u32 word0;
	u32 word4;
	u32 word8;
	u32 word12;
	u8 word16[32];
	u8 word48[32];

};
// -------- bitfields upiu_header --------

#define getp_upiu_header_TxnType(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 0))
#define read_upiu_header_TxnType(upiu_header)   (*getp_upiu_header_TxnType(upiu_header))
#define write_upiu_header_TxnType(upiu_header, v) do { \
  typeof(getp_upiu_header_TxnType(upiu_header)) p = getp_upiu_header_TxnType(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Transaction code
#define read_upiu_header_TxnType_txncode(upiu_header) \
  (((read_upiu_header_TxnType(upiu_header)) >> 0) & ((1ULL << 6) - 1))

#define write_upiu_header_TxnType_txncode(upiu_header, v) do { \
  typeof(getp_upiu_header_TxnType(upiu_header)) p = getp_upiu_header_TxnType(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 6) - 1) << 0)) | (((v) & ((1ULL << 6) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Whether end-to-end CRC of data segment is included
#define read_upiu_header_TxnType_DD(upiu_header) \
  (((read_upiu_header_TxnType(upiu_header)) >> 6) & ((1ULL << 1) - 1))

#define write_upiu_header_TxnType_DD(upiu_header, v) do { \
  typeof(getp_upiu_header_TxnType(upiu_header)) p = getp_upiu_header_TxnType(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 6)) | (((v) & ((1ULL << 1) - 1)) << 6); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Whether end-to-end CRC of all header segements are included
#define read_upiu_header_TxnType_HD(upiu_header) \
  (((read_upiu_header_TxnType(upiu_header)) >> 6) & ((1ULL << 1) - 1))

#define write_upiu_header_TxnType_HD(upiu_header, v) do { \
  typeof(getp_upiu_header_TxnType(upiu_header)) p = getp_upiu_header_TxnType(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 6)) | (((v) & ((1ULL << 1) - 1)) << 6); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_Flags(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 1))
#define read_upiu_header_Flags(upiu_header)   (*getp_upiu_header_Flags(upiu_header))
#define write_upiu_header_Flags(upiu_header, v) do { \
  typeof(getp_upiu_header_Flags(upiu_header)) p = getp_upiu_header_Flags(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_upiu_header_Flags_ATTR(upiu_header) \
  (((read_upiu_header_Flags(upiu_header)) >> 0) & ((1ULL << 2) - 1))

#define write_upiu_header_Flags_ATTR(upiu_header, v) do { \
  typeof(getp_upiu_header_Flags(upiu_header)) p = getp_upiu_header_Flags(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 2) - 1) << 0)) | (((v) & ((1ULL << 2) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Command priority
#define read_upiu_header_Flags_CP(upiu_header) \
  (((read_upiu_header_Flags(upiu_header)) >> 2) & ((1ULL << 1) - 1))

#define write_upiu_header_Flags_CP(upiu_header, v) do { \
  typeof(getp_upiu_header_Flags(upiu_header)) p = getp_upiu_header_Flags(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 2)) | (((v) & ((1ULL << 1) - 1)) << 2); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_upiu_header_Flags_D(upiu_header) \
  (((read_upiu_header_Flags(upiu_header)) >> 4) & ((1ULL << 1) - 1))

#define write_upiu_header_Flags_D(upiu_header, v) do { \
  typeof(getp_upiu_header_Flags(upiu_header)) p = getp_upiu_header_Flags(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 4)) | (((v) & ((1ULL << 1) - 1)) << 4); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_upiu_header_Flags_W(upiu_header) \
  (((read_upiu_header_Flags(upiu_header)) >> 5) & ((1ULL << 1) - 1))

#define write_upiu_header_Flags_W(upiu_header, v) do { \
  typeof(getp_upiu_header_Flags(upiu_header)) p = getp_upiu_header_Flags(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 5)) | (((v) & ((1ULL << 1) - 1)) << 5); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_upiu_header_Flags_U(upiu_header) \
  (((read_upiu_header_Flags(upiu_header)) >> 5) & ((1ULL << 1) - 1))

#define write_upiu_header_Flags_U(upiu_header, v) do { \
  typeof(getp_upiu_header_Flags(upiu_header)) p = getp_upiu_header_Flags(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 5)) | (((v) & ((1ULL << 1) - 1)) << 5); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_upiu_header_Flags_R(upiu_header) \
  (((read_upiu_header_Flags(upiu_header)) >> 6) & ((1ULL << 1) - 1))

#define write_upiu_header_Flags_R(upiu_header, v) do { \
  typeof(getp_upiu_header_Flags(upiu_header)) p = getp_upiu_header_Flags(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 6)) | (((v) & ((1ULL << 1) - 1)) << 6); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
#define read_upiu_header_Flags_O(upiu_header) \
  (((read_upiu_header_Flags(upiu_header)) >> 6) & ((1ULL << 1) - 1))

#define write_upiu_header_Flags_O(upiu_header, v) do { \
  typeof(getp_upiu_header_Flags(upiu_header)) p = getp_upiu_header_Flags(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 1) - 1) << 6)) | (((v) & ((1ULL << 1) - 1)) << 6); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_LUN(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 2))
#define read_upiu_header_LUN(upiu_header)   (*getp_upiu_header_LUN(upiu_header))
#define write_upiu_header_LUN(upiu_header, v) do { \
  typeof(getp_upiu_header_LUN(upiu_header)) p = getp_upiu_header_LUN(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_TskTag(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 3))
#define read_upiu_header_TskTag(upiu_header)   (*getp_upiu_header_TskTag(upiu_header))
#define write_upiu_header_TskTag(upiu_header, v) do { \
  typeof(getp_upiu_header_TskTag(upiu_header)) p = getp_upiu_header_TskTag(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_word4(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 4))
#define read_upiu_header_word4(upiu_header)   (*getp_upiu_header_word4(upiu_header))
#define write_upiu_header_word4(upiu_header, v) do { \
  typeof(getp_upiu_header_word4(upiu_header)) p = getp_upiu_header_word4(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Initiator ID
#define read_upiu_header_IID(upiu_header) \
  (((read_upiu_header_word4(upiu_header)) >> 4) & ((1ULL << 4) - 1))

#define write_upiu_header_IID(upiu_header, v) do { \
  typeof(getp_upiu_header_word4(upiu_header)) p = getp_upiu_header_word4(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 4) - 1) << 4)) | (((v) & ((1ULL << 4) - 1)) << 4); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Command Set Type
#define read_upiu_header_CmdSet(upiu_header) \
  (((read_upiu_header_word4(upiu_header)) >> 0) & ((1ULL << 4) - 1))

#define write_upiu_header_CmdSet(upiu_header, v) do { \
  typeof(getp_upiu_header_word4(upiu_header)) p = getp_upiu_header_word4(upiu_header); \
  (*p) = ((*p) & ~(((1ULL << 4) - 1) << 0)) | (((v) & ((1ULL << 4) - 1)) << 0); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_Function(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 5))
#define read_upiu_header_Function(upiu_header)   (*getp_upiu_header_Function(upiu_header))
#define write_upiu_header_Function(upiu_header, v) do { \
  typeof(getp_upiu_header_Function(upiu_header)) p = getp_upiu_header_Function(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_Response(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 6))
#define read_upiu_header_Response(upiu_header)   (*getp_upiu_header_Response(upiu_header))
#define write_upiu_header_Response(upiu_header, v) do { \
  typeof(getp_upiu_header_Response(upiu_header)) p = getp_upiu_header_Response(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_Status(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 7))
#define read_upiu_header_Status(upiu_header)   (*getp_upiu_header_Status(upiu_header))
#define write_upiu_header_Status(upiu_header, v) do { \
  typeof(getp_upiu_header_Status(upiu_header)) p = getp_upiu_header_Status(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
//  Total Extra Header Segement Length in 32-bit units

#define getp_upiu_header_TotEHSLen(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 8))
#define read_upiu_header_TotEHSLen(upiu_header)   (*getp_upiu_header_TotEHSLen(upiu_header))
#define write_upiu_header_TotEHSLen(upiu_header, v) do { \
  typeof(getp_upiu_header_TotEHSLen(upiu_header)) p = getp_upiu_header_TotEHSLen(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_DevInfo(upiu_header)   ((volatile u8 *)((char *)(upiu_header) + 9))
#define read_upiu_header_DevInfo(upiu_header)   (*getp_upiu_header_DevInfo(upiu_header))
#define write_upiu_header_DevInfo(upiu_header, v) do { \
  typeof(getp_upiu_header_DevInfo(upiu_header)) p = getp_upiu_header_DevInfo(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)

#define getp_upiu_header_DataSegLen(upiu_header)   ((volatile u16 *)((char *)(upiu_header) + 10))
#define read_upiu_header_DataSegLen(upiu_header)   (*getp_upiu_header_DataSegLen(upiu_header))
#define write_upiu_header_DataSegLen(upiu_header, v) do { \
  typeof(getp_upiu_header_DataSegLen(upiu_header)) p = getp_upiu_header_DataSegLen(upiu_header); \
  (*p = (v)); \
  /* __asm__ __volatile__("dc cvau, %0\n\t" : : "r" (p) :"memory"); */ \
  } while (0)
// -------- struct definitions of upiu_header --------
struct upiu_header {
	u8 TxnType;
	u8 Flags;
	u8 LUN;
	u8 TskTag;
	u8 word4;
	u8 Function;
	u8 Response;
	u8 Status;
	u8 TotEHSLen;
	u8 DevInfo;
	u16 DataSegLen;

};
