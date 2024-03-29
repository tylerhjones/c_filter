/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Joe Kopena, Drexel University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Joe Kopena <tjkopena@cs.drexel.edu>
 */

#include <iostream>
#include <iomanip>
#include <cstdlib> // added by me
#include <time.h>

#include <netinet/in.h>
extern "C" {
  #include <linux/netfilter.h>  /* Defines verdicts (NF_ACCEPT, etc) */
  #include <libnetfilter_queue/libnetfilter_queue.h>
}

using namespace std;

//----------------------------------------------------------------------
//------------------------------------------------------
static int Callback(nfq_q_handle *myQueue, struct nfgenmsg *msg,
                    nfq_data *pkt, void *cbData) {
  uint32_t id = 0;
  nfqnl_msg_packet_hdr *header;

  cout << "pkt recvd: "<< endl;

  // mine
  header = nfq_get_msg_packet_hdr(pkt);
  id = ntohl(header->packet_id);
  // end mine

  // if ((header = nfq_get_msg_packet_hdr(pkt))) {
  //   id = ntohl(header->packet_id);
  //   cout << "id " << id << "; hw_protocol " << setfill('0') << setw(4) <<
  //     hex << ntohs(header->hw_protocol) << "; hook " << ('0'+header->hook)
  //        << " ; ";
  // }

  // The HW address is only fetchable at certain hook points
  nfqnl_msg_packet_hw *macAddr = nfq_get_packet_hw(pkt);
  // if (macAddr) {
    // cout << "mac len " << ntohs(macAddr->hw_addrlen) << " addr ";
    // for (int i = 0; i < 8; i++) {
    //   cout << setfill('0') << setw(2) << hex << macAddr->hw_addr;
    // }
    // end if macAddr
  // } else {
  //   cout << "no MAC addr";
  // }

  // timeval tv;
  // if (!nfq_get_timestamp(pkt, &tv)) {
  //   cout << "; tstamp " << tv.tv_sec << "." << tv.tv_usec;
  // } else {
  //   cout << "; no tstamp";
  // }

  // cout << "; mark " << nfq_get_nfmark(pkt);

  // Note that you can also get the physical devices
  // cout << "; indev " << nfq_get_indev(pkt);
  // cout << "; outdev " << nfq_get_outdev(pkt);

  cout << endl;

  // Print the payload; in copy meta mode, only headers will be included;
  // in copy packet mode, whole packet will be returned.
  char *pktData;
  int len = nfq_get_payload(pkt, &pktData);
  cout << "lenght: " << len << endl;
  // if (len) {
  //   cout << "data[" << len << "]: '";
  //   for (int i = 0; i < len; i++) {
  //     if (isprint(pktData[i]))
  //       cout << pktData[i];
  //     else cout << " ";
  //   }
  //   cout << "'" << endl;
  //   // end data found
  // }

cout << endl << "before verdict given" << endl;
  // For this program we'll always accept the packet...
  return nfq_set_verdict(myQueue, id, NF_ACCEPT, 0, NULL);
  // end Callback
}

//----------------------------------------------------------------------
//------------------------------------------------------
int main(int argc, char **argv) {
  struct nfq_handle *nfqHandle;

  struct nfq_q_handle *myQueue;
  struct nfnl_handle *netlinkHandle;

  int fd, res;
  // char buf[8192]; it appears size change has no effect
  char buf[4096];

  // Get a queue connection handle from the module
  if (!(nfqHandle = nfq_open())) {
    cerr << "Error in nfq_open()" << endl;
    exit(-1);
  }

  // Unbind the handler from processing any IP packets
  // Not totally sure why this is done, or if it's necessary...
  if (nfq_unbind_pf(nfqHandle, AF_INET) < 0) {
    cerr << "Error in nfq_unbind_pf()" << endl;
    exit(1);
  }

  // Bind this handler to process IP packets...
  if (nfq_bind_pf(nfqHandle, AF_INET) < 0) {
    cerr << "Error in nfq_bind_pf()" << endl;
    exit(1);
  }

  // Install a callback on queue 0
  if (!(myQueue = nfq_create_queue(nfqHandle,  0, &Callback, NULL))) {
    cerr << "Error in nfq_create_queue()" << endl;
    exit(1);
  }

  // Turn on packet copy mode
  if (nfq_set_mode(myQueue, NFQNL_COPY_PACKET, 0xffff) < 0) {
    cerr << "Could not set packet copy mode" << endl;
    exit(1);
  }

  netlinkHandle = nfq_nfnlh(nfqHandle);
  fd = nfnl_fd(netlinkHandle);

  while ((res = recv(fd, buf, sizeof(buf), 0)) && res >= 0) {
    // I am not totally sure why a callback mechanism is used
    // rather than just handling it directly here, but that
    // seems to be the convention...
    nfq_handle_packet(nfqHandle, buf, res);
    // end while receiving traffic
  }

  nfq_destroy_queue(myQueue);

  nfq_close(nfqHandle);


  return 0;

  // end main
}