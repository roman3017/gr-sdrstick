/* -*- c++ -*- */
/* 
 * Code based upon gnuradio udp source module.
 * Changes Copyright 2012, 2013, 2014 Zephyr Engineering, Inc.
 * 
 * Copyright 2007,2008,2009,2010 Free Software Foundation, Inc.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#define HAVE_NETDB_H
#define HAVE_SYS_SOCKET_H
#define HAVE_NETINET_IN_H
#define HAVE_ARPA_INET_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "sdrstick_source_s_impl.h"
#include "sdrstick_cmds.h"
#include <gnuradio/math.h>
#include <stdexcept>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
typedef void* optval_t;

// ntohs() on FreeBSD may require both netinet/in.h and arpa/inet.h, in order
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#elif defined(HAVE_WINDOWS_H)
// if not posix, assume winsock
#define USING_WINSOCK
#include <winsock2.h>
#include <ws2tcpip.h>
#define SHUT_RDWR 2
typedef char* optval_t;
#endif

#define USE_SELECT    1  // non-blocking receive on all platforms
#define USE_RCV_TIMEO 0  // non-blocking receive on all but Cygwin
#define SRC_VERBOSE 0
#define EXPECTPAYSIZE 1408
#define SDR_DEBUG 0
#define START_FREQ 1580000

namespace gr {
  namespace sdrstick {

    sdrstick_source_s::sptr
    sdrstick_source_s::make(size_t itemsize,
                            const char *hostip,
                            const char *sipaddr,
                            int port,
                            int sr_idx
                            )

    {
      return gnuradio::get_initial_sptr
        (new sdrstick_source_s_impl(itemsize, hostip, sipaddr, port, sr_idx));

    }

    /*
     * The private constructor
     */
    sdrstick_source_s_impl::sdrstick_source_s_impl(size_t itemsize,
                                                   const char *hostip,
                                                   const char *sipaddr,
                                                   int port,
                                                   int sr_idx
                                                   )

      : gr::sync_block("sdrstick_source_s",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, itemsize)),
        d_itemsize(itemsize), 
        d_wait(0), d_pktnum(0), d_eof(0), d_connected(false), d_residual(0),
        d_pktcount(0), d_temp_offset(0), d_socket(-1), d_product(0)
    {
        int ret = 0;
        int rc = 0;
        char tmpbuf[1500];
        char hostname[256];
        hostent *host_entry;
        char *naddr;
        int opt_val;
        unsigned int tmpvar;
        char *rip;

        printf("sdrstick constructor\n");
        d_payload_size = EXPECTPAYSIZE;
        if(d_payload_size != EXPECTPAYSIZE) {
            report_error("sdrstick/init", "unexpected payload size");
        }
 
        // set up a socket for sdrstick discovery and command
        printf("setting up socket\n");

        struct sockaddr_in udp_cmdsin;
        struct sockaddr_in *d_cmdsin;
        unsigned int cmdsin_len = sizeof(sockaddr_in);
        char dscv_cmd[] = "SDRSTICK";
        unsigned int dscv_cmd_len = sizeof(dscv_cmd);
        d_cmdsin = &udp_cmdsin;


        printf("SDRstick debug: from flowgraph: host ip = %s, SDRSTICK ip = %s\n", hostip, sipaddr);
        printf("SDRstick debug: strlen(sipaddr) = %zd\n",strlen(sipaddr));
        gethostname(hostname,255);
        printf("SDRstick debug: hostname = %s\n",hostname);
 	    //TODO: this returns 127.0.1.1, use 2nd entry?
 	    host_entry = gethostbyname(hostname);
	    //naddr = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);
	    //printf("SDRstick debug: naddr = %s\n",naddr);
           
	    //clear the socket structure
	    memset((char *)&udp_cmdsin,0,sizeof(udp_cmdsin));	
	    udp_cmdsin.sin_family = AF_INET;
	    udp_cmdsin.sin_port = htons(8000);

        d_cmd_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if(d_cmd_socket <0) {
            report_error("sdrstick_udp_source", "can't open command socket");
		    printf("SDRstick debug: Unable to open command socket\n");
        }
	    printf("SDRstick debug: cmd_socket open: 0x%x\n",d_cmd_socket);

        //TODO: implement discovery retry
        //if we are not given the sdrstick ip address, we send an SDRSTICK Discovery packet to get the ip address.
        // unicast discovery is a future enhancement. For now we must always broadcast a discovery packet.
	    if(1) {
	    //if(strlen(sipaddr)==0)	{
		    udp_cmdsin.sin_addr.s_addr = inet_addr("255.255.255.255");	
            opt_val = 1;
            if(setsockopt(d_cmd_socket, SOL_SOCKET, SO_BROADCAST, (optval_t)&opt_val, sizeof(int)) == -1) {
              report_error("SO_BROADCAST","can't set socket option SO_BROADCAST");
            }
            //send SDRSTICK discovery packet
		    printf("SDRstick debug: Sending SDRstick discovery packet\n");
            if((rc = sendto(d_cmd_socket,dscv_cmd,dscv_cmd_len,0,(struct sockaddr *)d_cmdsin,cmdsin_len))==-1) {
                report_error("sdrstick_udp_source", "Error sending discovery packet");
                printf("SDRstick debug: sendto returns: %d\n",rc);
            }
            //receive SDRSTICK discovery response and capture sdrstick ip address in structure
            if((rc = recvfrom(d_cmd_socket, tmpbuf, sizeof(tmpbuf), 0, (struct sockaddr *)&udp_cmdsin, &cmdsin_len))==-1) {
                report_error("sdrstick_udp_source", "Error receiving discovery response");
                printf("SDRstick debug: discovery recvfrom returns: %d\n",rc);
                printf("SDRstick debug: Received from: %s, port: %d\n",						//verification
                 inet_ntoa(udp_cmdsin.sin_addr), ntohs(udp_cmdsin.sin_port));
            }
            else {
                printf("SDRstick debug: discovery recvfrom returns: %d\n",rc);
                printf("SDRstick debug: Received from: %s, port: %d\n",						//verification
                 	inet_ntoa(udp_cmdsin.sin_addr), ntohs(udp_cmdsin.sin_port));
                // verify response and set d_connected
                printf("SDRstick debug: Received data = %s\n",tmpbuf);
                if(strcmp(tmpbuf,"SDRSTICK")==0) {
                    d_connected = 1;
                }
                else {
                    d_connected = 0;
                }
            }
        }
        else {
            //set sdrstick ip address using string from GRC
            //udp_cmdsin.sin_addr.s_addr = inet_addr("192.168.1.25");
            udp_cmdsin.sin_addr.s_addr = inet_addr(sipaddr);
        }
        
        //testing to see if source ip address was filled in
        //rip = inet_ntoa((struct in_addr)udp_cmdsin.sin_addr);
        //printf("ip = %s\n",rip);
        
        // Turn on reuse address
        opt_val = 1;
        if(setsockopt(d_cmd_socket, SOL_SOCKET, SO_REUSEADDR, (optval_t)&opt_val, sizeof(int)) == -1) {
          report_error("SO_REUSEADDR","can't set command socket option SO_REUSEADDR");
        }

        //connect so we can use send() for commands
        rc = connect(d_cmd_socket, (struct sockaddr *) &udp_cmdsin, sizeof(udp_cmdsin));
        printf("SDRstick debug: connect returns %d\n",rc);

	    //read the firmware revision (for future use and backwards compatibility)
	    tmpvar = get_fwrev();
	    printf("SDRSTICK firmware revision = %u\n",tmpvar);

        d_product = get_product();
        printf("Product code = %d\n",d_product);
        
        //set frequency to default starting frequency
        set_freq(START_FREQ);

	    // -------------- init radio data consumer --------------------------------------- //
        #if defined(USING_WINSOCK) // for Windows (with MinGW)
        // initialize winsock DLL
        WSADATA wsaData;
        int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
        if( iResult != NO_ERROR ) {
            report_error( "sdrstick_udp_source WSAStartup", "can't open socket" );
        }
        #endif

        d_temp_buff = new char[d_payload_size];   // allow it to hold up to payload_size bytes

        //TODO: future enhancement allow user to enter specific host ip adr for multi networks
        //create a socket for receiving rx data
        struct sockaddr_in rxds;
        if((d_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
	        report_error("sdrstick_source_s","unable to open rx data socket");
        }

        //set parameters for the rx data structure
        rxds.sin_family = AF_INET;
        rxds.sin_port = htons(port);
        rxds.sin_addr.s_addr = htonl(INADDR_ANY);
        //name the socket
        if(bind(d_socket,(struct sockaddr *)&rxds,sizeof(sockaddr_in)) < 0) {
	        report_error("sdrstick_source_s", "bind failed");
        }
        //naddr = inet_ntoa((struct in_addr)rxds.sin_addr);
        //printf("SDRstick debug: naddr = %s\n",naddr);


        // So that we can re-bind to it without TIME_WAIT problems 
        opt_val = 1;
        if(setsockopt(d_socket, SOL_SOCKET, SO_REUSEADDR, &opt_val,sizeof(int))==-1)
            report_error("SO_REUSEADDR","can't set socket option SO_REUSEADDR");

        // Don't wait when shutting down
        linger lngr;
        lngr.l_onoff  = 1;
        lngr.l_linger = 0;
        if(setsockopt(d_socket, SOL_SOCKET, SO_LINGER, (optval_t)&lngr, sizeof(linger)) == -1) {
         if( !is_error(ENOPROTOOPT) ) {  // no SO_LINGER for SOCK_DGRAM on Windows
              report_error("SO_LINGER","can't set socket option SO_LINGER");
            }
         }

        #if USE_RCV_TIMEO
        // Set a timeout on the receive function to not block indefinitely
        // This value can (and probably should) be changed
        // Ignored on Cygwin
        #if defined(USING_WINSOCK)
        DWORD timeout = 1000;  // milliseconds
        #else
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        #endif
        if(setsockopt(d_socket, SOL_SOCKET, SO_RCVTIMEO, (optval_t)&timeout, sizeof(timeout)) == -1) {
         report_error("SO_RCVTIMEO","can't set socket option SO_RCVTIMEO");
         }
        #endif // USE_RCV_TIMEO

        //set sample rate to high
        set_samp_rate(sr_idx);
        //send run command to sdrstick
        set_run();

    }


    /*
     * Our virtual destructor.
     */
    sdrstick_source_s_impl::~sdrstick_source_s_impl()
    {

       if(d_cmd_socket != -1) {
	    if(d_connected) {
		    set_stop();
		    d_connected = 0;
	    }
	    shutdown(d_cmd_socket, SHUT_RDWR);
	    ::close(d_cmd_socket);
      }

      if (d_socket != -1){
        shutdown(d_socket, SHUT_RDWR);
    #if defined(USING_WINSOCK)
        closesocket(d_socket);
    #else
        ::close(d_socket);
    #endif
        d_socket = -1;
    #if defined(USING_WINSOCK) // for Windows (with MinGW)
       // free winsock resources
       WSACleanup();
    #endif
      }
       printf("\nbye bye\n");

    }

    int
    sdrstick_source_s_impl::is_error( int perr )
    {
        return( perr == errno );
    }

    void
    sdrstick_source_s_impl::report_error( const char *msg1, const char *msg2 )
    {
      // Deal with errors
      perror(msg1);
      if( msg2 != NULL )
        throw std::runtime_error(msg2);
      return;
    }

    void
    sdrstick_source_s_impl::set_freq(int freq) {
        unsigned int cmd[4] = {CMD_FREQ,0,0,0};
        unsigned int cmd_len = sizeof(cmd);
        int rc;
        unsigned int rxbuff[8];

        printf("set freq %d\n",freq);
        cmd[1] = freq;
        rc = send(d_cmd_socket, cmd, cmd_len, 0);
        //printf("set freq send returns %d\n",rc);
        //consume response
        rc = recv(d_cmd_socket, rxbuff, sizeof(rxbuff),0);
    }


    void
    sdrstick_source_s_impl::set_run(void) {
        unsigned int cmd[4] = {CMD_RUN,0,0,0};
        unsigned int cmd_len = sizeof(cmd);
        int rc;
        unsigned int rxbuff[8];

        printf("set run\n");
        rc = send(d_cmd_socket, cmd, cmd_len, 0);
        //printf("set run send returns %d\n",rc);
        //consume response
        rc = recv(d_cmd_socket, rxbuff, sizeof(rxbuff),0);
    }


    void
    sdrstick_source_s_impl::set_stop(void) {
        unsigned int cmd[4] = {CMD_STOP,0,0,0};
        unsigned int cmd_len = sizeof(cmd);
        int rc;
        unsigned int rxbuff[8];

        rc = send(d_cmd_socket, cmd, cmd_len, 0);
        //consume response
        rc = recv(d_cmd_socket, rxbuff, sizeof(rxbuff),0);

    }

    unsigned int
    sdrstick_source_s_impl::get_fwrev(void) {
        unsigned int cmd[4] = {CMD_VERS,0,0,0};
        unsigned int cmd_len = sizeof(cmd);
        int rc;
        unsigned int rxbuff[8];

        rc = send(d_cmd_socket, cmd, cmd_len, 0);
        rc = recv(d_cmd_socket, rxbuff, sizeof(rxbuff),0);
        //printf("rc = %d, fw rev = %d\n",rc,rxbuff[2]);
        return (rxbuff[2]);
    }

    unsigned int
    sdrstick_source_s_impl::get_product(void) {
        unsigned int cmd[4] = {CMD_PRODUCT,0,0,0};
        unsigned int cmd_len = sizeof(cmd);
        int rc;
        unsigned int rxbuff[8];

        rc = send(d_cmd_socket, cmd, cmd_len, 0);
        rc = recv(d_cmd_socket, rxbuff, sizeof(rxbuff),0);
        return (rxbuff[2]);
    }

    void
    sdrstick_source_s_impl::set_samp_rate(int sr_idx) {
        unsigned int cmd[4] = {CMD_SR,0,0,0};
        unsigned int cmd_len = sizeof(cmd);
        int rc;
        unsigned int rxbuff[8];

        printf("set sample rate %d:  %s\n",sr_idx, sr_idx ? "low":"high");
        cmd[1] = sr_idx;
        rc = send(d_cmd_socket, cmd, cmd_len, 0);
        //printf("set freq send returns %d\n",rc);
        //consume response
        rc = recv(d_cmd_socket, rxbuff, sizeof(rxbuff),0);
    }
        


    int
    sdrstick_source_s_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
       /* gr::thread::scoped_lock l(d_setlock);
        char *out = (char*)output_items[0];
        if(d_residual < 0)
          return -1;

        return 0;
       */
	    char *out = (char *) output_items[0];
	    ssize_t r=0, nbytes=0, bytes_received=0;
	    ssize_t total_bytes = (ssize_t)(d_itemsize*noutput_items);
	    ssize_t idx=0;
	    unsigned int *uiptr=(unsigned int *)d_temp_buff;
	    unsigned short *usptr=(unsigned short *)d_temp_buff;
	    unsigned int hdr_offset=0, size1, size2;
      

      // Remove items from temp buffer if they are in there
      if(d_residual) {
        nbytes = std::min(d_residual, total_bytes);
        memcpy(out, d_temp_buff+d_temp_offset, nbytes);
        bytes_received = nbytes;

        #if SRC_VERBOSE
            printf("Temp buff size: %ld  offset: %ld (bytes_received: %ld) (noutput_items: %d)\n",
	                d_residual, d_temp_offset, bytes_received, noutput_items);
        #endif

        // Increment pointer
        out += bytes_received;

        // Update indexing of amount of bytes left in the buffer
        d_residual -= nbytes;
        d_temp_offset += nbytes;

        // Return now with what we've got.
        assert(nbytes % d_itemsize == 0);
        return nbytes/d_itemsize;
      }

      while(1) {
        // get the data into our output buffer and record the number of bytes

        #if USE_SELECT
            // RCV_TIMEO doesn't work on all systems (e.g., Cygwin)
            // use select() instead of, or in addition to RCV_TIMEO
            fd_set readfds;
            timeval timeout;
            timeout.tv_sec = 1;	  // Init timeout each iteration.  Select can modify it.
            timeout.tv_usec = 0;
            FD_ZERO(&readfds);
            FD_SET(d_socket, &readfds);
            r = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
            if(r < 0) {
	        report_error("sdrstick_source/select",NULL);
	        return -1;
            }
            else if(r == 0 ) {  // timed out
              if( d_wait ) {
	        // Allow boost thread interrupt, then try again
	        boost::this_thread::interruption_point();
	        continue;
              }
              else
	        return -1;
            }
        #endif // USE_SELECT

        // This is a non-blocking call with a timeout set in the constructor
        r = recv(d_socket, d_temp_buff, d_payload_size, 0);  // get the entire payload or the what's available

        // Total udp packet payload is 1408 bytes. First 4 bytes is an integer packet number, next 4 are for future expansion.
        // Check packet number and strip off header before forwarding
	    // printf("r= %d, d_pktcount = %d\n",r,d_pktcount);
        if((d_pktcount == 0) && (r>=8)) { 					//at start of packet && received at least the header

		    //for(int i=0;i<64;i++)
		    //	printf("0x%04x ",*usptr++);
		    //printf("\n");

		    //skip the header when we copy out data
		    idx = 8;
	      									
		    if(r==d_payload_size)	//received entire packet
	      		d_pktcount = 0;
		    else
	        	d_pktcount = r;		//save byte count to locate header next time

		    if((d_pktnum !=0) && (*uiptr != d_pktnum+1)) { //if packets are lost
			    #if SDR_DEBUG
				    printf("Lost packet(s) %d to %d\n",d_pktnum+1,*uiptr-1);
			    #else
				    printf("u");
			    #endif
		    }
		    d_pktnum = *uiptr;			//save this packet number to check for missed packets next time
		    //printf("d_pktnum = %d\n",d_pktnum);
		    r= r-8;						//subtract off the header size from the received data count
        }
	    else { 	
		    //previous data was not a complete packet, data received in d_tmp_buff may have:
		    // 1)more data for that packet but not enough to complete the packet 
		    //     - increment d_pktcount, send all data
		    // 2)remainder of data in the packet							 
		    //     - increment d_pktcount (to 0 mod packetsize), send all data
		    // 3)remainder of data in packet plus start of next packet			 
		    //     - increment d_pktcount mod packetsize, delete header from middle of buffer
            printf("****** r= %zd, d_pktcount = %d\n",r,d_pktcount);

		    //Is there enough data in buffer to have received the next packet header
		    //FIXME - it is possible for the header to be split between reads as well. 
		    if((d_pktcount + r) > d_payload_size) {
			    //next header is at an offset of d_payload_size - d_pktcount
			    hdr_offset = d_payload_size - d_pktcount + 8;	//next packet data starts here
			    size1 = d_payload_size - d_pktcount;			//size of residual data from first packet
			    size2 = r - hdr_offset;							//size of data received in second packet
			    r = r - 8;										//subtract off the header size for sending calculations
		    }
		    else {
			    hdr_offset = 0;
		    }

		    d_pktcount = (d_pktcount + r) % d_payload_size;
	    }

        // If r > 0, round it down to a multiple of d_itemsize
        // (If sender is broken, don't propagate problem)
        if (r > 0)
		    r = (r/d_itemsize) * d_itemsize;

        // Check if there was a problem; forget it if the operation just timed out
        if(r == -1) {
		    if( is_error(EAGAIN) ) {  // handle non-blocking call timeout
            	#if SRC_VERBOSE
			        printf("UDP receive timed out\n");
           		#endif

			    if( d_wait ) {
	      			// Allow boost thread interrupt, then try again
	      			boost::this_thread::interruption_point();
	      			continue;
			    }
			    else
	      			return -1;
          	}
          	else {
			    report_error("udp_source/recv",NULL);
			    return -1;
          	}
        }
        else if(r==0) {
		    if(d_eof) {
			    // zero-length packet interpreted as EOF

			    #if SRC_VERBOSE
			        printf("\tzero-length packet received; returning EOF\n");
			    #endif

			    return -1;
          	}
         	else{
			    // do we need to allow boost thread interrupt?
			    boost::this_thread::interruption_point();
			    continue;
          	}
        }
        else {
		    // Calculate the number of bytes we can take from the buffer in this call
		    nbytes = std::min(r, total_bytes-bytes_received);

		    // adjust the total number of bytes we have to round down to nearest integer of an itemsize
		    nbytes -= ((bytes_received+nbytes) % d_itemsize);

		    // copy the number of bytes we want to look at here
		    if(hdr_offset) {
			    memcpy(out, d_temp_buff, size1);
			    memcpy(out, d_temp_buff+hdr_offset, size2);
		    }
		    else
			    memcpy(out, d_temp_buff+idx, nbytes);

          	d_residual = r - nbytes;                      // save the number of bytes stored
          	d_temp_offset=nbytes;                         // reset buffer index

          	// keep track of the total number of bytes received
          	bytes_received += nbytes;

          	// increment the pointer
          	out += nbytes;

          	// Immediately return when data comes in
          	break;
        }

        #if SNK_VERBOSE
        printf("\tbytes received: %d bytes (nbytes: %d)\n", bytes, nbytes);
        #endif
      }

      #if SRC_VERBOSE
      printf("Total Bytes Received: %ld (bytes_received / noutput_items = %ld / %d)\n",
	     bytes_received, bytes_received, noutput_items);
      #endif

      // bytes_received is already set to some integer multiple of itemsize
      return bytes_received/d_itemsize;       
    }

  } /* namespace sdrstick */
} /* namespace gr */

