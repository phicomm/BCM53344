/* Copyright 2003  All Rights Reserved. 

STP AF - An implementation of the STP address family sublayer for
Linux (See 802.1d)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version
2 of the License, or (at your option) any later version.
 
*/
 
#include <linux/autoconf.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kmod.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/ioctls.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/inetdevice.h>
#include <asm/uaccess.h>
#include <net/net_namespace.h>
#include <linux/nsproxy.h>
#include "l2_forwarder.h"
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_ioctl.h"
#include "br_api.h"
#include "bdebug.h"

/* List of all stp sockets. */
static struct hlist_head stp_sklist;
static rwlock_t stp_sklist_lock = RW_LOCK_UNLOCKED;
static atomic_t stp_socks_nr;
static const unsigned char group_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };
static const unsigned char rpvst_group_addr[6] = { 0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcd };

static struct proto _proto = {
        .name     = "STP",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};  

int stp_create (struct net *net, struct socket *sock, int protocol);
/* Private packet socket structures. */

static void
skb_dump (struct sk_buff *skb)
{
  unsigned int i, snap = 64;

  printk ("skb_dump (%p): from %s with len %d (%d) headroom=%d tailroom = %d\n ",
          skb, skb->dev ? skb->dev->name : "ip stack", skb->len,
          skb->truesize, skb_headroom (skb), skb_tailroom(skb));

#ifdef CONFIG_NET_SCHED
  printk ("mark %lu tcindex %d priority %d  \n", skb->mark,
          skb->tc_index, skb->priority);
#endif
  printk ("\n======= Data dump ===========\n");
  for (i = (unsigned int) skb->head; i <= (unsigned int) skb->tail; i++) {
    if (i == (unsigned int) skb->data)
      printk ("D");
    if (i == (unsigned int) skb->transport_header)
      printk ("R");
    if (i == (unsigned int) skb->network_header)
      printk ("N");
    if (i == (unsigned int) skb->mac_header)
      printk ("M");
    printk ("%02x", *((unsigned char *) i));
    if (0 == i % 8)
      printk (" ");
    if (0 == i % 32)
      printk ("\n");
    if (i == (unsigned int) skb->tail)
      printk ("T");
    if (i == snap)
      break;
  }
  printk ("\n======= Data dump end ============\n");
}

void 
stp_sock_destruct (struct sock *sk)
{
  BDEBUG ("STP socket %p free - %d are alive\n", sk, 
          atomic_read (&stp_socks_nr));
}

static struct proto_ops stp_ops;

/*
  This the bottom half of the bridge packet handling code.
  We do understand shared skbs. We do not register a packet handler
  because the bridge has grabbed all of the frames. We are really
  skipping the ip stack altogether.
*/

int 
stp_rcv (struct sk_buff * skb, struct apn_bridge * bridge,  
         struct net_device * port)
{
  struct sock * sk;
  br_frame_t frame_type;
  int skb_len = skb->len;
  u8 * skb_head = skb->data;
  struct sockaddr_igs * l2_skaddr;
  struct apn_bridge_port *apn_port;
  int pkt_type = *(skb->mac_header + 2 * ETH_ALEN) << 8 | 
    *(skb->mac_header + 2 * ETH_ALEN + 1);

  if (!bridge)
    {
      BDEBUG ("Bridge Pointer NULL - drop\n");
      return 0;
    }

  BR_READ_LOCK_BH (&stp_sklist_lock);

  apn_port = br_get_port (bridge, skb->dev->ifindex);

  if (apn_port == NULL)
    {
      BR_READ_UNLOCK_BH (&stp_sklist_lock);
      BDEBUG ("Port not found - drop\n");
      return 0;
    }

  if (apn_port->port_type == CUST_EDGE_PORT)
    {
      if (apn_port->proto_process.stp_process == APNBR_L2_PROTO_DISCARD)
        goto drop_n_acct;
      else if (apn_port->proto_process.stp_process == APNBR_L2_PROTO_TUNNEL)
        {
          br_flood_forward (bridge, skb, VLAN_NULL_VID,
                            apn_port->proto_process.stp_tun_vid, 0);
          BR_READ_UNLOCK_BH (&stp_sklist_lock);
          return 0;
        }
    }

  /* Is there room to schedule the packet */
  sk = hlist_empty (&stp_sklist) ? NULL : __sk_head (&stp_sklist);
  for (; sk; sk = sk_next (sk))
    {
      BDEBUG ("sock %p bridge %s port %s len %d\n", sk, 
              bridge->name, port->name, skb->len);

      if (atomic_read (&sk->sk_rmem_alloc) + skb->truesize >= (unsigned)sk->sk_rcvbuf)
        {
          continue;
        }

      /* Create a copy of the packet as we make change to it 
         for untagged frames. */
      {
        struct sk_buff *nskb = skb_copy (skb, GFP_ATOMIC);
        BDEBUG ("skb shared \n");

        if (nskb == NULL)
          goto drop_n_acct;

        skb = nskb;

        /* the original skb is freed by the caller. */
      }

      /* Fill out the l2_skaddr structure here */
      l2_skaddr = (struct sockaddr_igs *)&skb->cb[0];
      /* Indices first */
      l2_skaddr->port = skb->dev->ifindex;

      memcpy (&l2_skaddr->dest_mac[0], skb->mac_header, ETH_ALEN);

      if ((skb->dev->header_ops != NULL) && (skb->dev->header_ops->parse))
        {
          skb->dev->header_ops->parse (skb, l2_skaddr->src_mac);
        }
      else
        {
          memcpy (l2_skaddr->src_mac, skb->mac_header + ETH_ALEN, ETH_ALEN);
        }

      if (pkt_type < 1536)
        skb_trim (skb, pkt_type);

      l2_skaddr->vlanid = 0;

      if (memcmp (l2_skaddr->dest_mac, group_addr, 6) == 0
          && apn_port->port_type == PRO_NET_PORT)
        {
          frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_STAG);

          if (frame_type == UNTAGGED)
            {
              kfree (skb);
              BR_READ_UNLOCK_BH (&stp_sklist_lock);
              return 0;
            }

          l2_skaddr->vlanid = br_vlan_get_vid_from_frame (ETH_P_8021Q_STAG, skb);
          skb_pull (skb, VLAN_HEADER_LEN);
        }

     /* Extract header from tagged 802.1D and SSTP Bpdus */
     if ((!(memcmp (l2_skaddr->dest_mac, rpvst_group_addr, 6)) ||
           !(memcmp (l2_skaddr->dest_mac, group_addr, 6)) ) &&
           (bridge->type == BR_TYPE_RPVST) )
       {

         frame_type = br_vlan_get_frame_type(skb, ETH_P_8021Q_CTAG);

         if (frame_type != UNTAGGED)
           {
             l2_skaddr->vlanid = br_vlan_get_vid_from_frame(ETH_P_8021Q_CTAG, skb);
             skb_pull (skb, VLAN_HEADER_LEN);
           }
       }

      skb_set_owner_r (skb, sk);
      skb->dev = NULL;
      spin_lock (&sk->sk_receive_queue.lock);
      __skb_queue_tail (&sk->sk_receive_queue, skb);
      spin_unlock (&sk->sk_receive_queue.lock);
      sk->sk_data_ready (sk, skb->len);
    }

  BR_READ_UNLOCK_BH (&stp_sklist_lock);

  return 0;

 drop_n_acct:

  if (skb_head != skb->data && skb_shared (skb)) 
    {
      skb->data = skb_head;
      skb->len = skb_len;
    }

  BR_READ_UNLOCK_BH (&stp_sklist_lock);

  return 0;
}

static int 
stp_sendmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len)
{
  int hdrsz;
  bool_t tunnel_bpdu;
  unsigned char *pnt;
  unsigned char *addr;
  struct sk_buff *skb;
  unsigned short proto;
  struct apn_bridge *br;
  struct net_device *dev;
  struct sock *sk = sock->sk;
  unsigned char *start = NULL;
  struct apn_bridge_port *port;
  int ifindex, err, reserve = 0;
  short ethernet_type = ETH_P_802_3;
  struct sockaddr_igs * l2_skaddr = (struct sockaddr_igs *)msg->msg_name;
  const unsigned char pbb_group_addr[3]= { 0x01, 0x1e, 0x83 };
  int rpvst_support = BR_FALSE;

  tunnel_bpdu = BR_FALSE;

  /*
   *    Get and verify the address. 
   */

  BDEBUG ("sock %p msg %p len %d\n", sk, msg, len);
  if (l2_skaddr == NULL) 
    {
      BWARN ("l2_skaddr is null\n");
      return -EINVAL;
    }
  else 
    {
      err = -EINVAL;
      if (msg->msg_namelen < sizeof (struct sockaddr_igs))
        goto out;

      ifindex   = l2_skaddr->port;
      addr      = l2_skaddr->dest_mac;
    }
  proto = sk->sk_protocol;

  dev = dev_get_by_index (current->nsproxy->net_ns, ifindex);
  BDEBUG ("ifindex %d dev %p sock %p type %d proto %d\n", 
          ifindex, dev, sk, sock->type, proto);
  err = -ENXIO;
  if (dev == NULL)
    goto out_unlock;
  if (sock->type == SOCK_RAW)
    reserve = dev->hard_header_len;

  err = -EMSGSIZE;
  if (len > (dev->mtu + reserve))
    goto out_unlock;

  port = (struct apn_bridge_port *) dev->apn_fwd_port;

  if (port == NULL)
    goto out_unlock;

  br = port->br;

  if (br == NULL)
    goto out_unlock;

  /* Allocate work memory. */
  start = (unsigned char *) kmalloc (len, GFP_KERNEL);
  if (! start)
    goto out_unlock;

  /* Returns -EFAULT on error */
  err = memcpy_fromiovec (start, msg->msg_iov, len);
  if (err <0)
    goto out_unlock;

  hdrsz = ETH_HLEN + 3; /* DA MAC + SA MAC */

  if (((memcmp (addr, group_addr, 6) == 0) ||
      (memcmp(addr,pbb_group_addr,3) == 0))
      && port->port_type == PRO_NET_PORT)
    tunnel_bpdu = BR_TRUE;
  else if ((memcmp (addr, rpvst_group_addr, 6) == 0))
    {
      BDEBUG ("Native Vlan(%d) Vlan present in vlan tag(%d)\n",
               port->native_vid, l2_skaddr->vlanid);

      /* 1.On a 802.1q trunk BPDUs for the native vlan are carried untagged.
         2.On a 802.1q trunk BPDUs for vlan 1 (if not the native vlan) are
         carried with the 802.1q TAG, both the standard BPDUs and the SSTP BPDUs
         3.On a 802.1q trunk BPDUs of all the other vlans are carried with the
         802.1q TAG and the SSTP MAC address */

      if ((l2_skaddr->vlanid > VLAN_NULL_VID) &&
           (l2_skaddr->vlanid != port->native_vid))
        {
          tunnel_bpdu = BR_TRUE;
          rpvst_support = BR_TRUE;
        }

      if ((port->native_vid == 0) &&
          (l2_skaddr->vlanid == VLAN_DEFAULT_VID))
        tunnel_bpdu = BR_FALSE;
    }


  if (tunnel_bpdu == BR_TRUE)
    hdrsz += 4; /* Vlan Info */

  skb = sock_alloc_send_skb (sk, len+dev->hard_header_len + hdrsz, 
                             msg->msg_flags & MSG_DONTWAIT, &err);
  if (skb==NULL)
    goto out_unlock;

  skb_reserve (skb, (dev->hard_header_len + hdrsz) & ~hdrsz);
  skb->network_header = skb->data;

  skb->protocol = proto;
  skb->dev = dev;

  if ( tunnel_bpdu == BR_TRUE && rpvst_support != BR_TRUE)
    {
       if (!br_vlan_is_port_in_vlans_member_set (br,l2_skaddr->vlanid,
                                                 port->port_no))
         {
           BDEBUG ("port not in vlan member set\n");
           goto out_free;
         }

       eth_header (skb, dev, ethernet_type,
                   l2_skaddr->dest_mac, l2_skaddr->src_mac, len);

       skb->mac_header = skb->data;

       skb_pull (skb, ETH_HLEN);

       br_vlan_push_tag_header (skb, l2_skaddr->vlanid,
                                ETH_P_8021Q_STAG);
    }
  else if ( tunnel_bpdu == BR_TRUE && rpvst_support == BR_TRUE)
    {
       BDEBUG ("Adding tag port if vlan member set\n");

       if (!br_vlan_is_port_in_vlans_member_set (br,l2_skaddr->vlanid,
                                                 port->port_no))
         {
           BDEBUG ("port not in vlan member set\n");
           goto out_free;
         }
       eth_header (skb, dev, ethernet_type,
                   l2_skaddr->dest_mac, l2_skaddr->src_mac, len);

       skb->mac_header = skb->data;
       skb_pull (skb, ETH_HLEN);

       br_vlan_push_tag_header (skb, l2_skaddr->vlanid,
                                ETH_P_8021Q_CTAG);
    }
  else if ((dev->header_ops != NULL) && (dev->header_ops->create)) 
    {
      int res;
      err = -EINVAL;
      res = dev->header_ops->create (skb, dev, ethernet_type, addr, NULL, len);
      if (sock->type != SOCK_RAW) /* Changed from DGRAM */
        {
          skb->tail = skb->data;
          skb->len = 0;
        } 
      else if (res < 0)
        {
          goto out_free;
        }
    }
  else
    {
      /* Only ethernet encapsulation for now. */
      eth_header (skb, dev, ethernet_type,
                  l2_skaddr->dest_mac, l2_skaddr->src_mac, len);
    }

  /* Create space in buffer. */
  pnt = skb_put (skb, len);

  /* Just copy the contents passed. */ 
  memcpy (pnt, start, len);

  /* Free work memory. */
  kfree (start);
  start = NULL;

  err = -ENETDOWN;
  if (!(dev->flags & IFF_UP))
    goto out_free;

  /*
   *    Now send it
   */

  BDEBUG ("dev_queue_xmit called\n");
  err = dev_queue_xmit (skb);
  if (err > 0 && (err = net_xmit_errno (err)) != 0)
    goto out_unlock;

  dev_put (dev);

  return (len);

 out_free:
  if (start)
    {
      kfree (start);
      start = NULL;
    }
  kfree_skb (skb);
 out_unlock:
  if (start)
    {
      kfree (start);
      start = NULL;
    }
  if (dev)
    dev_put (dev);
 out:
  if (start)
    {
      kfree (start);
      start = NULL;
    }
  return err;
}

/*
 *      Close a STP socket. This is fairly simple. We immediately go
 *      to 'closed' state and remove our protocol entry in the device list.
 */

static int 
stp_release (struct socket *sock)
{
  struct sock *sk = sock->sk;
  struct sock **skp;
  BDEBUG ("sock %p\n", sk);

  if (!sk)
    return 0;

  BR_WRITE_LOCK_BH (&stp_sklist_lock);
  sk_del_node_init (sk);
  BR_WRITE_UNLOCK_BH (&stp_sklist_lock);

  /*
   *    Now the socket is dead. No more input will appear.
   */

  sock_orphan (sk);
  sock->sk = NULL;

  /* Purge queues */
  skb_queue_purge (&sk->sk_receive_queue);

  if (atomic_dec_and_test (&stp_socks_nr))
    {
      l2_mod_dec_use_count ();
    }

  sock_put (sk);

  return 0;
}


/*
 *      Create a packet of type SOCK_STP. 
 */

int 
stp_create (struct net *net, struct socket *sock, int protocol)
{
  struct sock *sk;

  BDEBUG ("protocol %d socket addr %p\n",protocol,sock);
  if (!capable (CAP_NET_RAW))
    return -EPERM;
  if (sock->type  != SOCK_RAW)
    return -ESOCKTNOSUPPORT;

  sock->state = SS_UNCONNECTED;

  sk = sk_alloc (net, PF_STP, GFP_KERNEL, &_proto);
  if (sk == NULL)
    {
      return -ENOBUFS;
    }
  BDEBUG ("sock %p protocol %d \n",sk, protocol);
  sock->ops = &stp_ops;  
  sock_init_data (sock,sk);

  sk->sk_family = PF_STP;
  sk->sk_protocol = protocol;
  sk->sk_destruct = stp_sock_destruct;

  if (atomic_read (&stp_socks_nr) == 0)
    {
      l2_mod_inc_use_count ();
    }
  atomic_inc (&stp_socks_nr);

  BR_WRITE_LOCK_BH (&stp_sklist_lock);
  sk_add_node (sk, &stp_sklist);
  BR_WRITE_UNLOCK_BH (&stp_sklist_lock);

  return 0;
}

/*
 *      Pull a packet from our receive queue and hand it to the user.
 *      If necessary we block.
 */

static int 
stp_recvmsg (struct kiocb *iocb, struct socket *sock, 
             struct msghdr *msg, size_t len,
             int flags)
{
  struct sock *sk;
  struct sk_buff *skb;
  int copied, err;

  err = -EINVAL;
  if (flags & ~(MSG_PEEK|MSG_DONTWAIT|MSG_TRUNC))
    return err;

  if ((sock == 0) || (msg == 0) || (iocb == 0) ||(sock->sk == 0 ))
    {
      BDEBUG("Invalid datagram received sock->sk (%p) \n",sock->sk);
      return err;
    }
  
  sk = sock->sk;
  BDEBUG ("RECVD MSD sock %p msg %p len %d flags 0x%x\n", sk, msg, len, flags);
  /*
   *    If the address length field is there to be filled in, we fill
   *    it in now.
   */

  msg->msg_namelen = sizeof (struct sockaddr_igs);

  /*
   *    Call the generic datagram receiver. This handles all sorts
   *    of horrible races and re-entrancy so we can forget about it
   *    in the protocol layers.
   *
   *    Now it will return ENETDOWN, if device have just gone down,
   *    but then it will block.
   */

  skb = skb_recv_datagram (sk, flags, flags & MSG_DONTWAIT, &err);

  /*
   *    An error occurred so return it. Because skb_recv_datagram () 
   *    handles the blocking we don't see and worry about blocking
   *    retries.
   */

  if (skb == NULL)
    return err;

  /*
   *    You lose any data beyond the buffer you gave. If it worries a
   *    user program they can ask the device for its MTU anyway.
   */

  copied = skb->len;
  if (copied > len)
    {
      copied=len;
      msg->msg_flags |= MSG_TRUNC;
    }

  err = skb_copy_datagram_iovec (skb, 0, msg->msg_iov, copied);
  if (err)
    {
      BDEBUG("Copy datagram_iovec failed %d\n", err);
      skb_free_datagram (sk, skb);
      return err;
    }

  sock_recv_timestamp (msg, sk, skb);

  if (msg->msg_name)
    memcpy (msg->msg_name, skb->cb, msg->msg_namelen);

  /*
   *    Free or return the buffer as appropriate. Again this
   *    hides all the races and re-entrancy issues from us.
   */
  err = (flags & MSG_TRUNC) ? skb->len : copied;

  skb_free_datagram (sk, skb);
  return err;
}

static struct proto_ops SOCKOPS_WRAPPED (stp_ops) = {
  family:       PF_STP,

  release:      stp_release,
  bind:         sock_no_bind,
  connect:      sock_no_connect,
  socketpair:   sock_no_socketpair,
  accept:       sock_no_accept,
  getname:      sock_no_getname,
  poll:         datagram_poll,
  ioctl:        sock_no_ioctl,
  listen:       sock_no_listen,
  shutdown:     sock_no_shutdown,
  setsockopt:   sock_no_setsockopt,
  getsockopt:   sock_no_getsockopt,
  sendmsg:      stp_sendmsg,
  recvmsg:      stp_recvmsg,
  mmap:         sock_no_mmap,
  sendpage:     sock_no_sendpage,
};

static struct net_proto_family stp_family_ops = {
  family:               PF_STP,
  create:               stp_create,
  owner:                THIS_MODULE,
};

void 
stp_exit (void)
{
  BWARN ("NET4: 7/19/2004 Linux 802.1d STP 1.0 for Net4.0 removed\n");
  sock_unregister (PF_STP);
  return;
}

int  
stp_init (void)
{
  BWARN ("NET4: 7/19/2004 Linux 802.1d STP 1.0 for Net4.0\n");
  sock_register (&stp_family_ops);
  return 0;
}
