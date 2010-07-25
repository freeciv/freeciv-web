<%@ page contentType="application/xrds+xml"%><?xml version="1.0" encoding="UTF-8"?>
<xrds:XRDS
  xmlns:xrds="xri://$xrds"
  xmlns:openid="http://openid.net/xmlns/1.0"
  xmlns="xri://$xrd*($v*2.0)">
  <XRD>
    <Service priority="0">
      <Type>http://openid.net/signon/1.0</Type>
      <URI><%= request.getScheme() + "://" + request.getServerName() + ":" + request.getServerPort()%>/simple-openid/provider.jsp</URI>
    </Service>
  </XRD>
</xrds:XRDS>
