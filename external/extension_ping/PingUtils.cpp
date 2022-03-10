
#include "PingUtils.h"
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <string>
#include <regex>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#pragma once

const std::regex IP4_REG("^([0-9]{1,3}\.){3}[0-9]{1,3}$");
/*
  This is somewhat arbitrary, imposes a max subdomain chain
  length of 4 and max TLD length of 6. Exact values here would
  probably depend on intended usage
*/
const std::regex DOMAIN_REG("^([a-zA-z]+\.){1,3}[a-zA-z]{1,6}$");
/*
  Arbitrary data attached to the ICMP request. Included below for the
  sake of tracing.
*/
const char* SEND_DATA = "PingUtils.cpp";
const ULONG DEFAULT_TIMEOUT = 10000;


/*
  @brief Makes an ICMP echo request to the host specified by HostName and returns the
		 latency in ms.

  @param[in] HostName - the name of the host (either in the format of an IPv4 address
						or domain name)
  @param[out] Latency - a pointer to a ULONG variable which will contain the number of ms
						the ICMP packet took to reach the remote host and return to the
						system, if the function succeeds

  @returns a ULONG representing the status of the function. see PingUtils.h

*/
_Check_return_
ULONG
PingUtils::GetPingLatency(
	_In_ const std::string HostName,
	_Out_ ULONG* Latency
)
{
	ULONG status;
	ULONG replyBufferSize;
	ULONG latency;
	HANDLE hIcmpFile;
	LPVOID icmpReply;
	PULONG destinationIp;
	PICMP_ECHO_REPLY pIcmpReply;

	destinationIp = NULL;
	status = STATUS_SUCCESS;
	icmpReply = NULL;
	hIcmpFile = INVALID_HANDLE_VALUE;
	replyBufferSize = 0;
	pIcmpReply = NULL;


	if (Latency == nullptr)
	{
		printf("Supplied Outptr Latency cannot be null\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	destinationIp = static_cast<PULONG>(malloc(sizeof(IN_ADDR)));

	if (destinationIp == NULL) {
		printf("Failed to allocate destinatin ip IN_ADDR struct. GLE: %d\n", GetLastError());
		status = STATUS_ALLOC_FAILED;
		goto Exit;
	}

	if (std::regex_search(HostName, IP4_REG))
	{
		status = PingUtils::GetIpAddrFromIpStr(HostName, destinationIp);
	}
	else if (std::regex_search(HostName, DOMAIN_REG))
	{
		status = PingUtils::GetIpAddrFromHostName(HostName, destinationIp);
	}
	else
	{
		status = STATUS_INVALID_URL;
		printf("Url %s does not match domain or IP4 regex.\n", HostName.c_str());
		goto Exit;
	}

	if (status != STATUS_SUCCESS)
	{
		printf("Failed to resolve IpAddr from Url with status %d\n", status);
		goto Exit;
	}

	hIcmpFile = IcmpCreateFile();

	if (hIcmpFile == INVALID_HANDLE_VALUE)
	{
		printf("Failed to create IcmpFile, GLE: %d\n", GetLastError());
		status = STATUS_ICMP_FILE_CREATION_FAILED;
		goto Exit;
	}

	replyBufferSize = sizeof(ICMP_ECHO_REPLY) + sizeof(SEND_DATA) + ICMP_ERROR_MSG_SIZE;
	icmpReply = malloc(replyBufferSize);
	if (icmpReply == NULL)
	{
		printf("Failed to allocate icmpReply buffer, GLE: %d\n", GetLastError());
		status = STATUS_ALLOC_FAILED;
		goto Exit;
	}

	status = IcmpSendEcho(
		hIcmpFile,
		/*
		  This param is formally an IpAddr (just an alias for ULONG) but
		  GetIpAddrFrom* returns IN_ADDR. With TCP IN_ADDR will effectively
		  be a wrapper around a ULONG so this cast is safe.
		*/
		*destinationIp,
		static_cast<LPVOID>(const_cast<char*>((SEND_DATA))),
		sizeof(SEND_DATA),
		NULL,
		icmpReply,
		replyBufferSize,
		DEFAULT_TIMEOUT
	);

	if (status == NULL)
	{
		printf("IcmpSendEcho failed. GLE: %d.\n", GetLastError());
		status = STATUS_PING_FAILED;
		goto Exit;
	}

	pIcmpReply = static_cast<PICMP_ECHO_REPLY>(icmpReply);
	*Latency = pIcmpReply->RoundTripTime;
	status = STATUS_SUCCESS;

Exit:

	if (icmpReply != NULL)
	{
		free(icmpReply);
	}
	if (hIcmpFile != INVALID_HANDLE_VALUE && hIcmpFile != NULL)
	{
		IcmpCloseHandle(hIcmpFile);
	}
	if (destinationIp != NULL)
	{
		free(destinationIp);
	}
	return status;
}

/*
  @brief Translates a std::string representing a domain name to an IpAddr struct

  @param[in] HostName - the name of the host (either in the format of an IPv4 address
						or domain name)
  @param[out] IpAddr - a pointer to a ULONG variable which will contain the IpAddr value

  @returns a ULONG representing the status of the function. see PingUtils.h

*/
_Check_return_
ULONG
PingUtils::GetIpAddrFromHostName(
	_In_ const std::string HostName,
	_Out_ ULONG* IpAddr
)
{
	ULONG status;
	ULONG* destinationIp;
	PADDRINFOA pIpAddrList;
	PADDRINFOA pAddrInfo;

	status = STATUS_INVALID_URL;
	pAddrInfo = NULL;
	pIpAddrList = NULL;
	destinationIp = NULL;

	if (!std::regex_search(HostName, DOMAIN_REG))
	{
		status = STATUS_INVALID_URL;
		goto Exit;
	}

	destinationIp = static_cast<PULONG>(malloc(sizeof(IN_ADDR)));

	if (destinationIp == NULL) {
		printf("Failed to allocate destinatin ip IN_ADDR struct. GLE: %d\n", GetLastError());
		status = STATUS_ALLOC_FAILED;
		goto Exit;
	}

	status = getaddrinfo(HostName.c_str(), NULL, NULL, &pIpAddrList);

	if (status != STATUS_SUCCESS || pIpAddrList == NULL)
	{
		printf("getaddrinfo failed with status %d, GLE: %d\n", status, GetLastError());
		status = STATUS_GETADDRINFO_FAILED;
		goto Exit;
	}

	for (pAddrInfo = pIpAddrList; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
	{
		/*
			Search through returns addresses, bail if we don't see an IP4 addr.
		*/
		if (pAddrInfo->ai_family == AF_INET)
		{
			/*
				sock_addr struct will be sock_addr_in struct if ai_family is AF_INET, so this
				should be safe. Not a great pattern though. I think MSDN mentioned a new struct
				to manage this more effectively. Would've liked to avoid this, just didn't have
				the time to dig around to find a better solution.
			*/
			*destinationIp = (reinterpret_cast<PSOCKADDR_IN>(pAddrInfo->ai_addr))->sin_addr.S_un.S_addr;
			status = STATUS_SUCCESS;
			break;
		}
	}

	if (status == STATUS_SUCCESS)
	{
		*IpAddr = *destinationIp;
	}

Exit:
	if (destinationIp != NULL)
	{
		free(destinationIp);
	}
	if (pIpAddrList != NULL)
	{
		freeaddrinfo(pIpAddrList);
	}
	return status;
}

/*
  @brief Translates a std::string representing an IPv4 address to an IpAddr struct

  @param[in] HostName - the name of the host (either in the format of an IPv4 address
						or domain name)
  @param[out] IpAddr - a pointer to a ULONG variable which will contain the IpAddr value

  @returns a ULONG representing the status of the function. see PingUtils.h

*/
_Check_return_
ULONG
PingUtils::GetIpAddrFromIpStr(
	_In_ const std::string IpStr,
	_Out_ ULONG* IpAddr
)
{
	ULONG status;
	ULONG* destinationIp;

	status = STATUS_INVALID_URL;
	destinationIp = NULL;

	if (!std::regex_search(IpStr, IP4_REG))
	{
		goto Exit;
	}

	/*
	  Invoking inet_pton with family AF_INET results destinationIp containing
	  an IN_ADDR struct
	*/
	destinationIp = static_cast<PULONG>(malloc(sizeof(IN_ADDR)));

	if (destinationIp == NULL) {
		printf("Failed to allocate destinatin ip IN_ADDR struct. GLE: %d\n", GetLastError());
		status = STATUS_ALLOC_FAILED;
		goto Exit;
	}

	status = inet_pton(AF_INET, IpStr.c_str(), destinationIp);

	if (status != INETPTON_SUCCESS)
	{
		printf("inet_pton failed with status %d, GLE: %d\n", status, GetLastError());
		status = STATUS_INETPON_FAILED;
		goto Exit;
	}

	status = STATUS_SUCCESS;
	*IpAddr = *destinationIp;

Exit:
	if (destinationIp != NULL)
	{
		free(destinationIp);
	}
	return status;
}