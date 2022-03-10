
#include <sal.h>
#include <string>
#include <winsock2.h>

#pragma once

#define STATUS_SUCCESS 0
#define STATUS_INETPON_FAILED 1
#define STATUS_INVALID_URL 2
#define STATUS_ICMP_FILE_CREATION_FAILED 3
#define STATUS_ALLOC_FAILED 4
#define STATUS_PING_FAILED 5
#define STATUS_GETADDRINFO_FAILED 6

/*
  inet_pton returns 1 on success
*/
#define INETPTON_SUCCESS 1

/*
  The size, in bytes, of an ICMP error message. Used to calculate
  the size of the reply buffer for IcmpSendEcho. Taken from IcmpAPI.h
*/
#define ICMP_ERROR_MSG_SIZE 8


namespace PingUtils {
	_Check_return_
	ULONG 
	GetPingLatency (
		_In_ const std::string NetUrl,
		_Out_ ULONG* Latency
	);

	_Check_return_
	ULONG
	GetIpAddrFromHostName(
		_In_ const std::string HostName,
		_Out_ ULONG* IpAddr
	);

	_Check_return_
	ULONG
	GetIpAddrFromIpStr(
		_In_ const std::string IpStr,
		_Out_ ULONG* IpAddr
	);
}

