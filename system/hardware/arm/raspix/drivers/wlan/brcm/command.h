#ifndef _COMMAND_H_
#define _COMMAND_H_

#define BRCMF_C_GET_VERSION         1
#define BRCMF_C_UP              2
#define BRCMF_C_DOWN                3
#define BRCMF_C_SET_PROMISC         10
#define BRCMF_C_GET_RATE            12
#define BRCMF_C_GET_INFRA           19
#define BRCMF_C_SET_INFRA           20
#define BRCMF_C_GET_AUTH            21
#define BRCMF_C_SET_AUTH            22
#define BRCMF_C_GET_BSSID           23
#define BRCMF_C_GET_SSID            25
#define BRCMF_C_SET_SSID            26
#define BRCMF_C_TERMINATED          28
#define BRCMF_C_GET_CHANNEL         29
#define BRCMF_C_SET_CHANNEL         30
#define BRCMF_C_GET_SRL             31
#define BRCMF_C_SET_SRL             32
#define BRCMF_C_GET_LRL             33
#define BRCMF_C_SET_LRL             34
#define BRCMF_C_GET_RADIO           37
#define BRCMF_C_SET_RADIO           38
#define BRCMF_C_GET_PHYTYPE         39
#define BRCMF_C_SET_KEY             45
#define BRCMF_C_GET_REGULATORY          46
#define BRCMF_C_SET_REGULATORY          47
#define BRCMF_C_SET_PASSIVE_SCAN        49
#define BRCMF_C_SCAN                50
#define BRCMF_C_SCAN_RESULTS            51
#define BRCMF_C_DISASSOC            52
#define BRCMF_C_REASSOC             53
#define BRCMF_C_SET_ROAM_TRIGGER        55
#define BRCMF_C_SET_ROAM_DELTA          57
#define BRCMF_C_GET_BCNPRD          75
#define BRCMF_C_SET_BCNPRD          76
#define BRCMF_C_GET_DTIMPRD         77
#define BRCMF_C_SET_DTIMPRD         78
#define BRCMF_C_SET_COUNTRY         84
#define BRCMF_C_GET_PM              85
#define BRCMF_C_SET_PM              86
#define BRCMF_C_GET_REVINFO         98
#define BRCMF_C_GET_MONITOR         107
#define BRCMF_C_SET_MONITOR         108
#define BRCMF_C_GET_CURR_RATESET        114
#define BRCMF_C_GET_AP              117
#define BRCMF_C_SET_AP              118
#define BRCMF_C_SET_SCB_AUTHORIZE       121
#define BRCMF_C_SET_SCB_DEAUTHORIZE     122
#define BRCMF_C_GET_RSSI            127
#define BRCMF_C_GET_WSEC            133
#define BRCMF_C_SET_WSEC            134
#define BRCMF_C_GET_PHY_NOISE           135
#define BRCMF_C_GET_BSS_INFO            136
#define BRCMF_C_GET_GET_PKTCNTS         137
#define BRCMF_C_GET_BANDLIST            140
#define BRCMF_C_SET_SCB_TIMEOUT         158
#define BRCMF_C_GET_ASSOCLIST           159
#define BRCMF_C_GET_PHYLIST         180
#define BRCMF_C_SET_SCAN_CHANNEL_TIME       185
#define BRCMF_C_SET_SCAN_UNASSOC_TIME       187
#define BRCMF_C_SCB_DEAUTHENTICATE_FOR_REASON   201
#define BRCMF_C_SET_ASSOC_PREFER        205
#define BRCMF_C_GET_VALID_CHANNELS      217
#define BRCMF_C_SET_FAKEFRAG            219
#define BRCMF_C_GET_KEY_PRIMARY         235
#define BRCMF_C_SET_KEY_PRIMARY         236
#define BRCMF_C_SET_SCAN_PASSIVE_TIME       258
#define BRCMF_C_GET_VAR             262
#define BRCMF_C_SET_VAR             263
#define BRCMF_C_SET_WSEC_PMK            268


int32_t brcmf_fil_iovar_data_set(int ifidx, char *name, const void *data, uint32_t len);
int brcmf_c_preinit_dcmds(void);
void scan(void);
int connect(const char*ssid, const char* pmk);
void get_ethaddr(char* mac);
#endif