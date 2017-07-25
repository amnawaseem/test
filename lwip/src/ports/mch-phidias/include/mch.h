#ifndef __MCH_H__
#define __MCH_H__

#include "lwipopts.h"
#include "arch/cc.h"
#include "lwip/err.h"

size_t printf(const char *format, ...);

extern void mch_abort(void);

sys_prot_t sys_arch_protect();
void sys_arch_unprotect(sys_prot_t val);

void memset(void *buf, u8_t val, u32_t size);
void memcpy(void *dest, const void *src, u32_t size);
s32_t memcmp(const u8_t *buf1, const u8_t *buf2, size_t size);
size_t strlen(const char *str);

u16_t lwip_htons(u16_t v);
u32_t lwip_htonl(u32_t v);
u16_t lwip_ntohs(u16_t v);
u32_t lwip_ntohl(u32_t v);

u32_t sys_now(void);
u32_t sys_rand(void);

// drivers

void drv_serial_outstr(const char *);
void drv_serial_outch(char);

#define	LINKSTATE_MEGOOD	1
#define	LINKSTATE_WEGOOD	2

struct netif_virtual_pkthdr {
	size_t size;
	size_t next_offset;
};

struct netif_virtual_control {
	u64_t linkstate;
	size_t producer_offset;
	size_t x_consumer_offset;
	u64_t session_nonce;
};

struct netif_virtual_buffer {
	void *base;
	size_t size;
	void *ptr;
};

struct netif_virtual_priv {
	struct netif_virtual_buffer rx;
	const struct netif_virtual_control *rx_control;
	struct netif_virtual_buffer tx;
	struct netif_virtual_control *tx_control;
};

struct netif;
struct pbuf;

err_t phidias_netif_virtual_init(struct netif *netif);
err_t phidias_netif_virtual_rx(struct netif *netif);
err_t phidias_netif_virtual_tx(struct netif *netif, struct pbuf *pkt);
void phidias_netif_virtual_poll(struct netif *netif);

void architecture_trigger(u32_t);
void architecture_init(void);
void go_to_sleep(void);

#endif /* __MCH_H__ */
