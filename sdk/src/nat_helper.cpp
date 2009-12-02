/* 
 * Copyright (C) 2009 Gang Liu <gangban.lau@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include "nat_helper.h"

#include "voxve.h"

#include "global.h"
#include "utils.h"
#include "channel.h"

#define THIS_FILE "nat_helper.cpp"

extern struct voxve_data voxve_var;

/** Enable STUN support **/
voxve_status_t voxve_stun_enable(const char * stun_server_addr)
{
	voxve_var.is_enable_stun = PJ_TRUE;
	pj_ansi_snprintf(voxve_var.stun_server_addr, sizeof(voxve_var.stun_server_addr), "%s", stun_server_addr);

	return 0;
}

pj_status_t stun_resolve_server(pj_bool_t wait)
{
	if (voxve_var.stun_status == PJ_EUNKNOWN)
	{
		/* Initialize STUN configuration */
		pj_stun_config_init(&voxve_var.stun_cfg, &voxve_var.cp.factory, 0,
				pjmedia_endpt_get_ioqueue(voxve_var.med_endpt), voxve_var.timer_heap);

		/* Start STUN server resolution */
		voxve_var.stun_status = PJ_EPENDING;

		/* TODO stun_domain not supported now */
		if (strlen(voxve_var.stun_server_addr) > 0)
		{
			pj_str_t stun_host = pj_str(voxve_var.stun_server_addr);
			pj_str_t str_host, str_port;
			int port;
			pj_hostent he;

			str_port.ptr = pj_strchr(&stun_host, ':');
			if (str_port.ptr != NULL) 
			{
				str_host.ptr = stun_host.ptr;
				str_host.slen = (str_port.ptr - str_host.ptr);
				str_port.ptr++;
				str_port.slen = stun_host.slen - str_host.slen - 1;
				port = (int)pj_strtoul(&str_port);
				if (port < 1 || port > 65535) 
				{
					logging(THIS_FILE, VOXVE_LOG_WARN, "Invalid STUN server", PJ_EINVAL);
					voxve_var.stun_status = PJ_EINVAL;
					return voxve_var.stun_status;
				}
			} 
			else
			{
				str_host = stun_host;
				port = 3478;
			}

			voxve_var.stun_status = 
				pj_sockaddr_in_init(&voxve_var.stun_srv.ipv4, &str_host, 
				    (pj_uint16_t)port);

			if (voxve_var.stun_status != PJ_SUCCESS) 
			{
				voxve_var.stun_status = pj_gethostbyname(&str_host, &he);

				if (voxve_var.stun_status == PJ_SUCCESS) 
				{
					pj_sockaddr_in_init(&voxve_var.stun_srv.ipv4, NULL, 0);
					voxve_var.stun_srv.ipv4.sin_addr = *(pj_in_addr*)he.h_addr;
					voxve_var.stun_srv.ipv4.sin_port = pj_htons((pj_uint16_t)port);
				}
				else
				{
					logging(THIS_FILE, VOXVE_LOG_WARN, "Invalid STUN server", voxve_var.stun_status);
					return voxve_var.stun_status;
				}
			}

			PJ_LOG(3,(THIS_FILE, 
				"STUN server %.*s resolved, address is %s:%d",
				(int)stun_host.slen, stun_host.ptr,
				pj_inet_ntoa(voxve_var.stun_srv.ipv4.sin_addr),
				(int)pj_ntohs(voxve_var.stun_srv.ipv4.sin_port)));
		}
		/* Otherwise disable STUN. */
		else
		{
			voxve_var.stun_status = PJ_SUCCESS;
		}

		return voxve_var.stun_status;
	}
	else if (voxve_var.stun_status == PJ_EPENDING)
	{
		/* STUN server resolution has been started, wait for the
		 * result.
	     */
		if (wait) 
		{
			while (voxve_var.stun_status == PJ_EPENDING)
				pj_thread_sleep(10);
		}

		return voxve_var.stun_status;
	}
	else
	{
		/* STUN server has been resolved, return the status */
		return voxve_var.stun_status;
	}
}

/* 
 * Create RTP and RTCP socket pair, and resolve their public
 * address via STUN.
 */
pj_status_t stun_create_rtp_rtcp_sock(pjmedia_sock_info *skinfo, unsigned short local_port)
{
	PJ_ASSERT_RETURN(voxve_var.is_enable_stun, PJ_FALSE);	

    int i;
    pj_sockaddr_in bound_addr;
    pj_sockaddr_in mapped_addr[2];
    pj_status_t status = PJ_SUCCESS;
    char addr_buf[PJ_INET6_ADDRSTRLEN+2];
    pj_sock_t sock[2];

    /* Make sure STUN server resolution has completed */
    status = stun_resolve_server(PJ_TRUE);
    if (status != PJ_SUCCESS) 
	{
		logging(THIS_FILE, VOXVE_LOG_WARN, "Error resolving STUN server", status);
		return status;
    }

    for (i=0; i<2; ++i)
		sock[i] = PJ_INVALID_SOCKET;

	bound_addr.sin_addr.s_addr = PJ_INADDR_ANY;

	/* Create and bind RTP socket. */
	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock[0]);
	if (status != PJ_SUCCESS) 
	{
	    logging(THIS_FILE, VOXVE_LOG_WARN, "socket() error", status);
	    return status;
	}

	status=pj_sock_bind_in(sock[0], pj_ntohl(bound_addr.sin_addr.s_addr), 
			       local_port);
	if (status != PJ_SUCCESS) 
	{
	    pj_sock_close(sock[0]); 
	    sock[0] = PJ_INVALID_SOCKET;
	    return status;
	}

	/* Create and bind RTCP socket. */
	status = pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sock[1]);
	if (status != PJ_SUCCESS) 
	{
	    logging(THIS_FILE, VOXVE_LOG_WARN, "socket() error", status);
	    pj_sock_close(sock[0]);
	    return status;
	}

	status=pj_sock_bind_in(sock[1], pj_ntohl(bound_addr.sin_addr.s_addr), 
			       (pj_uint16_t)(local_port+1));
	if (status != PJ_SUCCESS) 
	{
	    pj_sock_close(sock[0]); 
	    sock[0] = PJ_INVALID_SOCKET;

	    pj_sock_close(sock[1]); 
	    sock[1] = PJ_INVALID_SOCKET;
	    return status;
	}

	if (voxve_var.stun_srv.addr.sa_family != 0)
	{
	    char ip_addr[32];
	    pj_str_t stun_srv;

	    pj_ansi_strcpy(ip_addr, 
			   pj_inet_ntoa(voxve_var.stun_srv.ipv4.sin_addr));
	    stun_srv = pj_str(ip_addr);

	    status=pjstun_get_mapped_addr(&voxve_var.cp.factory, 2, sock,
					   &stun_srv, pj_ntohs(voxve_var.stun_srv.ipv4.sin_port),
					   &stun_srv, pj_ntohs(voxve_var.stun_srv.ipv4.sin_port),
					   mapped_addr);
	    if (status != PJ_SUCCESS) 
		{
			logging(THIS_FILE, VOXVE_LOG_WARN, "STUN resolve error", status);
			goto on_error;
	    }

	}

    if (sock[0] == PJ_INVALID_SOCKET) 
	{
		PJ_LOG(1,(THIS_FILE, "Unable to find appropriate RTP/RTCP ports combination"));
		goto on_error;
    }

    skinfo->rtp_sock = sock[0];
    pj_memcpy(&skinfo->rtp_addr_name, &mapped_addr[0], sizeof(pj_sockaddr_in));

    skinfo->rtcp_sock = sock[1];
    pj_memcpy(&skinfo->rtcp_addr_name, &mapped_addr[1], sizeof(pj_sockaddr_in));

    PJ_LOG(4,(THIS_FILE, "RTP socket reachable at %s",
	      pj_sockaddr_print(&skinfo->rtp_addr_name, addr_buf,
				sizeof(addr_buf), 3)));
    PJ_LOG(4,(THIS_FILE, "RTCP socket reachable at %s",
	      pj_sockaddr_print(&skinfo->rtcp_addr_name, addr_buf,
				sizeof(addr_buf), 3)));

	return PJ_SUCCESS;

on_error:
    for (i=0; i<2; ++i) 
	{
		if (sock[i] != PJ_INVALID_SOCKET)
			pj_sock_close(sock[i]);
    }
    return status;
}

/** Get resolved public address via STUN **/
voxve_status_t voxve_stun_get_public_addr(int channel_id, char * addr_buf, unsigned buf_len, unsigned &port)
{
	PJ_ASSERT_RETURN(voxve_var.is_enable_stun, PJ_FALSE);

	channel_t * channel = channel_find(channel_id);
	if (channel != NULL)
	{
		pj_sockaddr_print(&channel->skinfo.rtp_addr_name, addr_buf, buf_len, 0);
		port = pj_sockaddr_get_port(&channel->skinfo.rtp_addr_name);

		return PJ_SUCCESS;
	}
	else
		return PJ_FALSE;
}
