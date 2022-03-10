# osquery - extension_ping 

 
This repo contains an osquery extension which retrieves the latency for a ping request to either an IPv4 or domain URL. 


## Build and Execute 
 

The extension can be built by cloning this repository and following the steps to build osquery from source included at https://osquery.readthedocs.io/en/stable/development/building/. Notably, if all pre-reqs have been installed

```
# Using a PowerShell console as Administrator (see note, below)
# Download source
git clone https://github.com/osquery/osquery
cd osquery

# Configure
mkdir build; cd build
cmake -G "Visual Studio 16 2019" -A x64 ..

# Build
cmake --build . --config RelWithDebInfo -j10 # Number of projects to build in parallel
```

Once built the ping_extension table can be exeucted with osqueryi  

``` 

.\osqueryi.exe --extension ..\..\external\RelWithDebInfo\external_extension_ping.ext.exe --allow_unsafe 

```

The above assumes osquery was built with the RelWithDebInfo config and was executed from build\osquery\RelWithDebInfo 


--allow_unsafe allows osquery to load the extension from a user-writable directory. This can be omitted if the directory which holds the external_extension_ping.ext.exe is owned by admin. 


## extension_ping Design 


The design for the table is fairly simple as much of the complexity for the task involves making the ICMP echo request and retrieving the reported latency. In it's current form the extension will make an ICMP echo request to every host specified in the "host" parameter of the query, reporting the latency in ms. There is no attempt at caching, partially because there are already some caching components baked in to osquery itself, and partially because there wasn't time to implement a coherent caching scheme.


It may have been wise to implement a small in-memory caching scheme using std::map, or perhaps best would be to have some sort of persistent caching scheme that might have been used to create a true ping "table" using a persistent store like the registry. This level of complexity would likely only be warranted if there were greater volume/performance demands for the table, as this does sort of break the osquery charter of virtual data stores. 

 
Internally latency is retrieved by calling IcmpSendEcho. Most work here is fairly straightforward, there's a bit of fiddling with WinSock structs and some painful address resolution methods, but for the most part the GetPingLatency method is fairly simple. It would be better to retry on failure in extension_ping\main.cpp since it's possible IcmpSendEcho will time out (the default timeout provided is 10s), but this was omitted as I just didn't have the time to add it. 

 
## Testing

The PingUtils::GetPingLatency function is covered in ad-hoc unit tests included in https://github.com/NWc0de/PingStats (PingStats.cpp). It would've been much better to use a framework like GTest but alas I didn't have time to mock them up properly, so I had to ship the ad-hoc tests I used in VS Studio while working the PingUtils methods. These can be built and executed via VS Studio on any recent Windows host (tested with VS Studio 2019).