/* ====================================================================

   Copyright (c) 2018 Juergen Liegner  All rights reserved.
   (https://www.mikrocontroller.net/topic/444994)

   Copyright (c) 2019 Thorsten Godau (dl9sec)
   (Created an Arduino library from the original code and did some beautification)
   
   Copyright (c) 2020 Ralf Lehmann
   (added Registration to SIP protocol)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

   3. Neither the name of the author(s) nor the names of any contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.

   ====================================================================*/
#include <MD5Builder.h>

#include "ArduinoSIP.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Hardware and API independent Sip class
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

Sip::Sip(char *pBuf, size_t lBuf) {

  pbuf = pBuf;
  lbuf = lBuf;
  pDialNr = "";
  pDialDesc = "";

}


Sip::~Sip() {

}


void Sip::Init(const char *SipIp, int SipPort, const char *MyIp, int MyPort, const char *SipUser, const char *SipPassWd, int RegTimeout, int MaxDialSec) {

  Udp.begin(SipPort);

  caRead[0] = 0;
  pbuf[0] = 0;
  pSipIp = SipIp;
  iSipPort = SipPort;
  pSipUser = SipUser;
  pSipPassWd = SipPassWd;
  pMyIp = MyIp;
  iMyPort = MyPort;
  iAuthCnt = 0;
  iRingTime = 0;
  iRegTime = RegTimeout;
  iMaxTime = MaxDialSec * 1000;
}

void Sip::setCallCallback(void (*callback)(const char * from)){
  fcalCallback = callback;
}


bool Sip::Dial(const char *DialNr, const char *DialDesc) {

  if ( iRingTime )
    return false;

  iDialRetries = 0;
  pDialNr = DialNr;
  pDialDesc = DialDesc;
  Invite();
  iDialRetries++;
  iRingTime = Millis();

  return true;
}


void Sip::Processing(char *pBuf, size_t lBuf) {

  int packetSize = Udp.parsePacket();

  if ( packetSize > 0 )
  {
    pBuf[0] = 0;
    packetSize = Udp.read(pBuf, lBuf);
    if ( packetSize > 0 )
    {
      pBuf[packetSize] = 0;


      IPAddress remoteIp = Udp.remoteIP();
      log_d("\r\n----- read %i bytes from: %s:%i ----\r\n", (int)packetSize, remoteIp.toString().c_str(), Udp.remotePort());
      log_d("Buf:\n%s", pBuf);
      log_d("----------------------------------------------------\r\n");

    }
  }

  HandleUdpPacket((packetSize > 0) ? pBuf : 0 );
}


void Sip::HandleUdpPacket(const char *p) {

  uint32_t iWorkTime = iRingTime ? (Millis() - iRingTime) : 0;

  if ( iRingTime && iWorkTime > iMaxTime )
  {
    // Cancel(3);
    Bye(3);
    iRingTime = 0;
  }

  if ( !p )
  {
    // max 5 dial retry when loos first invite packet
    if ( iAuthCnt == 0 && iDialRetries < 5 && iWorkTime > (iDialRetries * 200) )
    {
      iDialRetries++;
      delay(30);
      Invite();
    }

    return;
  }

  if ( strstr(p, "SIP/2.0 401 Unauthorized") == p )
  {

    if ( strstr(p, "INVITE")) {
      Ack(p);
      // call Invite with response data (p) to build auth md5 hashes
      Invite(p);
    }
    if ( strstr(p, "REGISTER")) {

      // call Register with response data (p) to build auth md5 hashes
      Register(p);
    }
  }
  else if ( strstr(p, "INVITE") == p ) { // incoming call
    log_d("Incoming call");
    char from[256];
    ParseParameter(from, 256, "From:", p, '\n');
    if ( fcalCallback ){
      (*fcalCallback)(from);
    }
    //ParseReturnParams(p);
  }
  else if ( strstr(p, "BYE") == p )
  {
    Ok(p);
    iRingTime = 0;
  }
  else if ( strstr(p, "SIP/2.0 200") == p )   // OK
  {
    ParseReturnParams(p);
    if ( !strstr(p, "REGISTER")) { // No ACK needed on registration
      Ack(p);
    }
  }
  else if (    strstr(p, "SIP/2.0 183 ") == p   // Session Progress
               || strstr(p, "SIP/2.0 180 ") == p ) // Ringing
  {
    ParseReturnParams(p);
  }
  else if ( strstr(p, "SIP/2.0 100 ") == p )  // Trying
  {
    ParseReturnParams(p);
    Ack(p);
  }
  else if (    strstr(p, "SIP/2.0 486 ") == p   // Busy Here
               || strstr(p, "SIP/2.0 603 ") == p   // Decline
               || strstr(p, "SIP/2.0 487 ") == p)  // Request Terminatet
  {
    Ack(p);
    iRingTime = 0;
  }
  else if (strstr(p, "INFO") == p)
  {
    iLastCSeq = GrepInteger(p, "\nCSeq: ");
    Ok(p);
  }

}


void Sip::AddSipLine(const char* constFormat , ... ) {

  va_list arglist;
  va_start(arglist, constFormat);
  uint16_t l = (uint16_t)strlen(pbuf);
  char *p = pbuf + l;
  vsnprintf(p, lbuf - l, constFormat, arglist );
  va_end(arglist);
  l = (uint16_t)strlen(pbuf);
  if ( l < (lbuf - 2) )
  {
    pbuf[l] = '\r';
    pbuf[l + 1] = '\n';
    pbuf[l + 2] = 0;
  }
}


// Search a line in response date (p) and append on pbuf
bool Sip::AddCopySipLine(const char *p, const char *psearch) {

  char *pa = strstr((char*)p, psearch);

  if ( pa )
  {
    char *pe = strstr(pa, "\r");

    if ( pe == 0 )
      pe = strstr(pa, "\n");

    if ( pe > pa )
    {
      char c = *pe;
      *pe = 0;
      AddSipLine("%s", pa);
      *pe = c;

      return true;
    }
  }

  return false;
}


// Parse parameter value from http formated string
bool Sip::ParseParameter(char *dest, int destlen, const char *name, const char *line, char cq) {

  const char *qp;
  const char *r;

  if ( ( r = strstr(line, name) ) != NULL )
  {
    r = r + strlen(name);
    qp = strchr(r, cq);
    int l = qp - r;
    if ( l < destlen )
    {
      strncpy(dest, r, l);
      dest[l] = 0;

      return true;
    }
  }

  return false;
}


// Copy Call-ID, From, Via and To from response to caRead using later for BYE or CANCEL the call
bool Sip::ParseReturnParams(const char *p) {

  pbuf[0] = 0;

  AddCopySipLine(p, "Call-ID: ");
  AddCopySipLine(p, "From: ");
  AddCopySipLine(p, "Via: ");
  AddCopySipLine(p, "To: ");

  if ( strlen(pbuf) >= 2 )
  {
    strcpy(caRead, pbuf);
    caRead[strlen(caRead) - 2] = 0;
  }

  return true;
}


int Sip::GrepInteger(const char *p, const char *psearch) {

  int param = -1;
  const char *pc = strstr(p, psearch);

  if ( pc )
  {
    param = atoi(pc + strlen(psearch));
  }

  return param;
}


void Sip::Subscribe() {
  Register();
}


void Sip::Ack(const char *p) {

  char ca[32];

  bool b = ParseParameter(ca, (int)sizeof(ca), "To: <", p, '>');

  if ( !b )
    return;

  pbuf[0] = 0;
  AddSipLine("ACK %s SIP/2.0", ca);
  AddCopySipLine(p, "Call-ID: ");
  int cseq = GrepInteger(p, "\nCSeq: ");
  AddSipLine("CSeq: %i ACK",  cseq);
  AddCopySipLine(p, "From: ");
  AddCopySipLine(p, "Via: ");
  AddCopySipLine(p, "To: ");
  AddSipLine("Content-Length: 0");
  AddSipLine("");
  SendUdp();
}


void Sip::Cancel(int cseq) {

  if ( caRead[0] == 0 )
    return;

  pbuf[0] = 0;
  AddSipLine("%s sip:%s@%s SIP/2.0",  "CANCEL", pDialNr, pSipIp);
  AddSipLine("%s",  caRead);
  AddSipLine("CSeq: %i %s",  cseq, "CANCEL");
  AddSipLine("Max-Forwards: 70");
  AddSipLine("User-Agent: sip-client/0.0.1");
  AddSipLine("Content-Length: 0");
  AddSipLine("");
  SendUdp();
}


void Sip::Bye(int cseq) {

  if ( caRead[0] == 0 )
    return;

  pbuf[0] = 0;
  AddSipLine("%s sip:%s@%s SIP/2.0",  "BYE", pDialNr, pSipIp);
  AddSipLine("%s",  caRead);
  AddSipLine("CSeq: %i %s", cseq, "BYE");
  AddSipLine("Max-Forwards: 70");
  AddSipLine("User-Agent: sip-client/0.0.1");
  AddSipLine("Content-Length: 0");
  AddSipLine("");
  SendUdp();
}


void Sip::Ok(const char *p) {

  pbuf[0] = 0;
  AddSipLine("SIP/2.0 200 OK");
  AddCopySipLine(p, "Call-ID: ");
  AddCopySipLine(p, "CSeq: ");
  AddCopySipLine(p, "From: ");
  AddCopySipLine(p, "Via: ");
  AddCopySipLine(p, "To: ");
  AddSipLine("Content-Length: 0");
  AddSipLine("");
  SendUdp();
}


// Call invite without or with the response from peer
void Sip::Invite(const char *p) {

  // prevent loops
  if ( p && iAuthCnt > 3 )
    return;

  // using caRead for temp. store realm and nonce
  char *caRealm = caRead;
  char *caNonce = caRead + 128;

  char *haResp = 0;
  int   cseq = 1;

  if ( !p )
  {
    iAuthCnt = 0;

    if ( iDialRetries == 0 )
    {
      callid = Random();
      tagid = Random();
      branchid = Random();
    }
  }
  else
  {
    cseq = 2;

    if (    ParseParameter(caRealm, 128, " realm=\"", p)
            && ParseParameter(caNonce, 128, " nonce=\"", p) )
    {
      // using output buffer to build the md5 hashes
      // store the md5 haResp to end of buffer
      char *ha1Hex = pbuf;
      char *ha2Hex = pbuf + 33;
      haResp = pbuf + lbuf - 34;
      char *pTemp = pbuf + 66;

      snprintf(pTemp, lbuf - 100, "%s:%s:%s", pSipUser, caRealm, pSipPassWd);
      MakeMd5Digest(ha1Hex, pTemp);

      snprintf(pTemp, lbuf - 100, "INVITE:sip:%s@%s", pDialNr, pSipIp);
      MakeMd5Digest(ha2Hex, pTemp);

      snprintf(pTemp, lbuf - 100, "%s:%s:%s", ha1Hex, caNonce, ha2Hex);
      MakeMd5Digest(haResp, pTemp);
    }
    else
    {
      caRead[0] = 0;

      return;
    }
  }

  pbuf[0] = 0;
  AddSipLine("INVITE sip:%s@%s SIP/2.0", pDialNr, pSipIp);
  AddSipLine("Call-ID: %010u@%s",  callid, pMyIp);
  AddSipLine("CSeq: %i INVITE",  cseq);
  AddSipLine("Max-Forwards: 70");
  // not needed for fritzbox
  // AddSipLine("User-Agent: sipdial by jl");
  AddSipLine("From: \"%s\"  <sip:%s@%s>;tag=%010u", pDialDesc, pSipUser, pSipIp, tagid);
  AddSipLine("Via: SIP/2.0/UDP %s:%i;branch=%010u;rport=%i", pMyIp, iMyPort, branchid, iMyPort);
  AddSipLine("To: <sip:%s@%s>", pDialNr, pSipIp);
  AddSipLine("Contact: \"%s\" <sip:%s@%s:%i;transport=udp>", pSipUser, pSipUser, pMyIp, iMyPort);

  if ( p )
  {
    // authentication
    AddSipLine("Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"sip:%s@%s\", response=\"%s\"", pSipUser, caRealm, caNonce, pDialNr, pSipIp, haResp);
    iAuthCnt++;
  }

  AddSipLine("Content-Type: application/sdp");
  // not needed for fritzbox
  // AddSipLine("Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
  AddSipLine("Content-Length: 0");
  AddSipLine("");
  caRead[0] = 0;
  SendUdp();
}

// Call register without or with the response from peer
// registration needs to be renewed; best after iRegTime/2 seconds
// it is normal that the first registration call is denied with 401 unauthorized
void Sip::Register(const char *p) {

  log_d("SIP Register %s data", p ? "with" : "without");
  // prevent loops
  if ( p && iAuthCnt > 3 )
    return;

  // using caRead for temp. store realm and nonce
  char *caRealm = caRead;
  char *caNonce = caRead + 128;

  char *haResp = 0;
  int   cseq = 1;

  if ( !p )
  {
    iAuthCnt = 0;

    if ( iDialRetries == 0 )
    {
      callid = Random();
      tagid = Random();
      branchid = Random();
    }
  }
  else
  {
    cseq = 2;

    if (    ParseParameter(caRealm, 128, " realm=\"", p)
            && ParseParameter(caNonce, 128, " nonce=\"", p) )
    {
      // using output buffer to build the md5 hashes
      // store the md5 haResp to end of buffer
      char *ha1Hex = pbuf;
      char *ha2Hex = pbuf + 33;
      haResp = pbuf + lbuf - 34;
      char *pTemp = pbuf + 66;

      snprintf(pTemp, lbuf - 100, "%s:%s:%s", pSipUser, caRealm, pSipPassWd);
      MakeMd5Digest(ha1Hex, pTemp);

      //snprintf(pTemp, lbuf - 100, "REGISTER:sip:%s@%s", pDialNr, pSipIp);
      snprintf(pTemp, lbuf - 100, "REGISTER:sip:%s@%s", pSipUser, pSipIp);
      MakeMd5Digest(ha2Hex, pTemp);

      snprintf(pTemp, lbuf - 100, "%s:%s:%s", ha1Hex, caNonce, ha2Hex);
      MakeMd5Digest(haResp, pTemp);
    }
    else
    {
      caRead[0] = 0;

      return;
    }
  }

  pbuf[0] = 0;
  AddSipLine("REGISTER sip:%s SIP/2.0", pMyIp);
  AddSipLine("Call-ID: %010u@%s",  callid, pMyIp);
  AddSipLine("CSeq: %i REGISTER",  cseq);
  AddSipLine("Max-Forwards: 70");
  // not needed for fritzbox
  // AddSipLine("User-Agent: sipdial by jl");
  AddSipLine("From: \"%s\"  <sip:%s@%s>;tag=%010u", "Tardis", pSipUser, pSipIp, tagid);
  AddSipLine("Via: SIP/2.0/UDP %s:%i;branch=%010u;rport=%i", pMyIp, iMyPort, branchid, iMyPort);
  AddSipLine("To: <sip:%s@%s>", pSipUser, pSipIp);
  AddSipLine("Contact: \"%s\" <sip:%s@%s:%i;transport=udp>", pSipUser, pSipUser, pMyIp, iMyPort);
  extern int SipEXPIRES;
  AddSipLine("Expires: %d", iRegTime);


  if ( p )
  {
    // authentication
    AddSipLine("Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"sip:%s@%s\", response=\"%s\"", pSipUser, caRealm, caNonce, pSipUser, pSipIp, haResp);
    iAuthCnt++;
  }

  AddSipLine("Content-Type: application/sdp");
  // not needed for fritzbox
  //AddSipLine("Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
  AddSipLine("Content-Length: 0");
  AddSipLine("");
  caRead[0] = 0;
  SendUdp();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Hardware dependent interface functions
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t Sip::Millis() {

  return (uint32_t)millis() + 1;
}


// Generate a 30 bit random number
uint32_t Sip::Random() {

  return ((((uint32_t)rand()) & 0x7fff) << 15) + ((((uint32_t)rand()) & 0x7fff));
  //return secureRandom(0x3fffffff);
}


int Sip::SendUdp() {

  Udp.beginPacket(pSipIp, iSipPort);
  Udp.write((uint8_t *) pbuf, strlen(pbuf));
  Udp.endPacket();
  log_d("\r\n----- send %i bytes -----------------------\r\n%s", strlen(pbuf), pbuf);
  log_d("------------------------------------------------\r\n");


  return 0;
}


void Sip::MakeMd5Digest(char *pOutHex33, char *pIn) {

  MD5Builder aMd5;

  aMd5.begin();
  aMd5.add(pIn);
  aMd5.calculate();
  aMd5.getChars(pOutHex33);
}
