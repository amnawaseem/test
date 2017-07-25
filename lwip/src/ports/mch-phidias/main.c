#include "mch.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "netif/ethernet.h"

struct netif virtnet;
struct netif_virtual_priv virtnetpriv;
// struct netif extnet;

static err_t bitbucket_recv(void *ctx, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
	LWIP_DEBUGF(MCH_APP_DEBUG, ("BitBucket Conn Recv % % % %\r\n", ctx, pcb, p, err));
	if (p) {
		tcp_recved(pcb, p->tot_len);
		pbuf_free(p);
	}
	if (!p && (err == ERR_OK)) {
		tcp_close(pcb);
	}
	return ERR_OK;
}
static err_t bitbucket_sent(void *ctx, struct tcp_pcb *pcb, u16_t len) {
	LWIP_DEBUGF(MCH_APP_DEBUG, ("BitBucket Conn Sent % % %\r\n", ctx, pcb, len));
	return ERR_OK;
}
static void bitbucket_err(void *ctx, err_t err) {}
static err_t bitbucket_accept(void *ctx, struct tcp_pcb *newpcb, err_t err) {
	LWIP_DEBUGF(MCH_APP_DEBUG, ("BitBucket Conn Accept % % %\r\n", ctx, newpcb, err));

	tcp_recv(newpcb, bitbucket_recv);
	tcp_sent(newpcb, bitbucket_sent);
	tcp_err(newpcb, bitbucket_err);
	tcp_arg(newpcb, newpcb);

	return ERR_OK;
}

int main(void) {
	ip_addr_t ipaddr, ipmask, gwaddr;
	struct tcp_pcb *bitbucket_pcb;
    
	printf("LWIP @ Phidias\r\n");

	lwip_init();

	printf(" * LWIP Core initialized.\r\n");

	architecture_init();

	ip_addr_set_ip4_u32(&ipaddr, PP_HTONL(0x0a0a0a02));
	ip_addr_set_ip4_u32(&ipmask, PP_HTONL(0xffffff00));
	ip_addr_set_ip4_u32(&gwaddr, IPADDR_ANY);

	virtnetpriv.rx.base = (void *)0xfee00000;
	virtnetpriv.rx.size = 0x00100000;
	virtnetpriv.tx.base = (void *)0xfef00000;
	virtnetpriv.tx.size = 0x00100000;

	netif_add(&virtnet, ip_2_ip4(&ipaddr), ip_2_ip4(&ipmask), ip_2_ip4(&gwaddr),
		&virtnetpriv, &phidias_netif_virtual_init, &ethernet_input);

	netif_set_up(&virtnet);

	printf(" * Phidias NetIF added.\r\n");

	bitbucket_pcb = tcp_new();

	(void)tcp_bind(bitbucket_pcb, &ipaddr, 9); // 0x3333);
	bitbucket_pcb = tcp_listen_with_backlog(bitbucket_pcb, 1);
	tcp_accept(bitbucket_pcb, bitbucket_accept);

	printf(" * Bitbucket TCP socket established.\r\n");

	printf("Entering main loop.\r\n");
	while (1) {
		phidias_netif_virtual_poll(&virtnet);
		go_to_sleep();
	}
}

void mch_abort(void) {
	printf("MCH ABORT %\n", __builtin_return_address(0));
	while (1) { go_to_sleep(); }
}
