#include "mch.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/err.h"

//

static void phidias_netif_virtual_notify(struct netif *netif) {
	architecture_trigger(0);
}

static void phidias_netif_virtual_reset_hard(struct netif *netif) {
	netif_set_link_down(netif);
	// priv teardown
}

static void phidias_netif_virtual_poll_linkstate(struct netif *netif) {
	struct netif_virtual_priv *priv = netif->state;

	/* Case 1: the interface is already up */
	/* Check 1a: check if the other side disappeared */

	if (netif_is_link_up(netif)) {
		if (priv->tx_control->session_nonce != priv->rx_control->session_nonce) {
			priv->tx_control->linkstate = LINKSTATE_MEGOOD;
			phidias_netif_virtual_reset_hard(netif);
			phidias_netif_virtual_notify(netif);
		}
		return;
	}

	/* Case 2: the interface is down */
	/* Case 2a: other side has adopted our nonce and already upped itself */

	if ((priv->rx_control->linkstate == LINKSTATE_WEGOOD) &&
		(priv->rx_control->session_nonce == priv->tx_control->session_nonce)) {
		priv->tx_control->linkstate = LINKSTATE_WEGOOD;
		netif_set_link_up(netif);
		phidias_netif_virtual_notify(netif);
		return;
	}

	/* Case 2b: other side has the higher nonce; adopt and up ourselves */

	if ((priv->rx_control->linkstate == LINKSTATE_MEGOOD) &&
		(priv->rx_control->session_nonce > priv->tx_control->session_nonce)) {
		priv->tx_control->session_nonce = priv->rx_control->session_nonce;
		priv->tx_control->linkstate = LINKSTATE_WEGOOD;
		netif_set_link_up(netif);
		phidias_netif_virtual_notify(netif);
		return;
	}

	/* Case 2c: we're still in handshake mode; no changes */
}

//

static void rxringbuf_copy_out(struct netif_virtual_priv *priv,
		void *dst, size_t ring_offset, u32_t size) {
	if (ring_offset + size <= priv->rx.size) {
		memcpy(dst, priv->rx.base + ring_offset, size);
	} else {
		u32_t size1 = priv->rx.size - ring_offset;
		memcpy(dst, priv->rx.base + ring_offset, size1);
		memcpy(dst + size1, priv->rx.base + sizeof(struct netif_virtual_control), size - size1);
	}
}

static size_t txringbuf_copy_in(struct netif_virtual_priv *priv,
		size_t ring_offset, const void *src, u32_t size) {
	if (ring_offset + size <= priv->tx.size) {
		memcpy(priv->tx.base + ring_offset, src, size);
		return ring_offset + size;
	} else {
		u32_t size1 = priv->tx.size - ring_offset;
		memcpy(priv->tx.base + ring_offset, src, size1);
		memcpy(priv->tx.base + sizeof(struct netif_virtual_control), src + size1, size - size1);
		return sizeof(struct netif_virtual_control) + size - size1;
	}
}

//

err_t phidias_netif_virtual_rx(struct netif *netif) {
	struct netif_virtual_priv *priv = netif->state;
	const struct netif_virtual_pkthdr *phdr;

	if (!netif_is_link_up(netif))
		return ERR_IF;

	LWIP_DEBUGF(MCH_DRIVER_DEBUG | LWIP_DBG_FRESH, ("RX % %\r\n", priv->rx_control->producer_offset,
		priv->tx_control->x_consumer_offset));

	while (priv->rx_control->producer_offset != priv->tx_control->x_consumer_offset) {
		struct pbuf *pkt = NULL;

		phdr = priv->rx.base + priv->tx_control->x_consumer_offset;

		if (phdr->size > netif->mtu) {
			printf("RX drop, invalid packet size % (exceeds MTU %)\n", phdr->size, netif->mtu);
			phidias_netif_virtual_reset_hard(netif);
			return ERR_VAL;
		}

		pkt = pbuf_alloc(PBUF_RAW, phdr->size, PBUF_POOL);
		pkt->tot_len = pkt->len = phdr->size;
		rxringbuf_copy_out(priv, pkt->payload,
			priv->tx_control->x_consumer_offset + sizeof(struct netif_virtual_pkthdr),
			phdr->size);

		LWIP_DEBUGF(MCH_DRIVER_DEBUG | LWIP_DBG_FRESH, ("RX off % size %\r\n",
			priv->tx_control->x_consumer_offset, pkt->tot_len));
		//printf("Phidias NetIF: rx packet size %\r\n", pkt->tot_len);
		netif->input(pkt, netif);

		if (phdr->next_offset + sizeof(struct netif_virtual_pkthdr) > priv->rx.size) {
			printf("RX abort, invalid next_offset (beyond/too close to bounds)\n");
			phidias_netif_virtual_reset_hard(netif);
			return ERR_VAL;
		}

		priv->tx_control->x_consumer_offset = phdr->next_offset;
	}

	return ERR_OK;
}

err_t phidias_netif_virtual_tx(struct netif *netif, struct pbuf *pkt) {
	struct netif_virtual_priv *priv = netif->state;
	struct netif_virtual_pkthdr *phdr;
	size_t space_used;

	if (!netif_is_link_up(netif)) {
		printf("TX drop, no link\r\n");
		return ERR_IF;
	}

	phdr = priv->tx.base + priv->tx_control->producer_offset;
	space_used = (priv->tx.size + priv->tx_control->producer_offset - priv->rx_control->x_consumer_offset) % priv->tx.size;

	if (priv->tx.size - space_used < sizeof(struct netif_virtual_pkthdr) + pkt->tot_len) {
		printf("TX drop, buffer overrun\n");
		return ERR_VAL;
	}

	if (pkt->next) {
		printf("Help, chained PBuf\r\n");
		return ERR_VAL;
	}

	phdr->size = pkt->tot_len;
	phdr->next_offset = txringbuf_copy_in(priv,
		priv->tx_control->producer_offset + sizeof(struct netif_virtual_pkthdr),
		pkt->payload, pkt->tot_len);

	if (phdr->next_offset & (MEM_ALIGNMENT-1)) {
		phdr->next_offset += MEM_ALIGNMENT - ((phdr->next_offset) % MEM_ALIGNMENT);
	}

	if (phdr->next_offset + sizeof(struct netif_virtual_pkthdr) > priv->tx.size) {
		phdr->next_offset = sizeof(struct netif_virtual_control);
	}
	LWIP_DEBUGF(MCH_DRIVER_DEBUG | LWIP_DBG_FRESH, ("TX, off % --> %\r\n",
		priv->tx_control->producer_offset, phdr->next_offset));
	//printf("Phidias NetIF: tx packet size %\r\n", pkt->tot_len);

	priv->tx_control->producer_offset = phdr->next_offset;
	phidias_netif_virtual_notify(netif);

	return ERR_OK;
}

//

void phidias_netif_virtual_poll(struct netif *netif) {
	phidias_netif_virtual_poll_linkstate(netif);

	if (netif_is_link_up(netif)) {
		phidias_netif_virtual_rx(netif);
	}
}

//

err_t phidias_netif_virtual_init(struct netif *netif) {
	struct netif_virtual_priv *priv = netif->state;
	u8_t hwaddr_base[6] = { 0x50, 0x56, 0x4e, 0x00, 0x05, 0x06 };

	netif->mtu = 1514;
	netif->flags |= NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP;
	netif->output = &etharp_output;
	netif->linkoutput = &phidias_netif_virtual_tx;

	netif->hwaddr_len = ETH_HWADDR_LEN;
	memcpy(netif->hwaddr, hwaddr_base, ETH_HWADDR_LEN);
	netif->hwaddr[3] += netif->num;

	priv->rx_control = priv->rx.base;
	priv->tx_control = priv->tx.base;

	priv->tx_control->producer_offset = sizeof(struct netif_virtual_control);
	priv->tx_control->x_consumer_offset = sizeof(struct netif_virtual_control);
	priv->tx_control->session_nonce = LWIP_RAND();
	priv->tx_control->session_nonce *= priv->tx_control->session_nonce;
	priv->tx_control->linkstate = LINKSTATE_MEGOOD;

	printf("Phidias NetIF Init, Nonce %\r\n", priv->tx_control->session_nonce);

	return ERR_OK;
}
