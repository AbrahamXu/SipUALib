#pragma once

#ifndef AV_RTP_SESSION_H
#define AV_RTP_SESSION_H

#include <jrtplib3/rtpsession.h>
#include "Worker.h"


extern jrtplib::RTPMemoryManager* GetRTPMemMgr();


class AVRTPSession : public jrtplib::RTPSession, public CWorker
{
    typedef jrtplib::RTPSession super;
public:
    AVRTPSession(jrtplib::RTPRandom *rnd = 0, jrtplib::RTPMemoryManager *mgr = ::GetRTPMemMgr());
    virtual ~AVRTPSession(void);

    // Override to prevent sending if stopped working
    int SendPacket(const void *data,size_t len,
        uint8_t pt,bool mark,uint32_t timestampinc);

protected:
	/** Allocate a user defined transmitter.
	 *  In case you specified in the Create function that you want to use a
	 *  user defined transmitter, you should override this function. The RTPTransmitter 
	 *  instance returned by this function will then be used to send and receive RTP and 
	 *  RTCP packets. Note that when the session is destroyed, this RTPTransmitter 
	 *  instance will be destroyed as well.
 	 */
	virtual jrtplib::RTPTransmitter *NewUserDefinedTransmitter()						{ return 0; }
	
	/** Is called when an incoming RTP packet is about to be processed. */
	virtual void OnRTPPacket(jrtplib::RTPPacket *pack,const jrtplib::RTPTime &receivetime,
	                         const jrtplib::RTPAddress *senderaddress);

	/** Is called when an incoming RTCP packet is about to be processed. */
	virtual void OnRTCPCompoundPacket(jrtplib::RTCPCompoundPacket *pack,const jrtplib::RTPTime &receivetime,
	                                  const jrtplib::RTPAddress *senderaddress);

	/** Is called when an SSRC collision was detected. 
	 *  Is called when an SSRC collision was detected. The instance \c srcdat is the one present in 
	 *  the table, the address \c senderaddress is the one that collided with one of the addresses 
	 *  and \c isrtp indicates against which address of \c srcdat the check failed.
	 */
	virtual void OnSSRCCollision(jrtplib::RTPSourceData *srcdat,const jrtplib::RTPAddress *senderaddress,bool isrtp)	{ }

	/** Is called when another CNAME was received than the one already present for source \c srcdat. */
	virtual void OnCNAMECollision(jrtplib::RTPSourceData *srcdat,const jrtplib::RTPAddress *senderaddress,
	                              const uint8_t *cname,size_t cnamelength)				{ }

	/** Is called when a new entry \c srcdat is added to the source table. */
	virtual void OnNewSource(jrtplib::RTPSourceData *srcdat)			 				;

	/** Is called when the entry \c srcdat is about to be deleted from the source table. */
	virtual void OnRemoveSource(jrtplib::RTPSourceData *srcdat)						;
	
	/** Is called when participant \c srcdat is timed out. */
	virtual void OnTimeout(jrtplib::RTPSourceData *srcdat)							;

	/** Is called when participant \c srcdat is timed after having sent a BYE packet. */
	virtual void OnBYETimeout(jrtplib::RTPSourceData *srcdat)						;

	/** Is called when an RTCP APP packet \c apppacket has been received at time \c receivetime 
	 *  from address \c senderaddress.
	 */
	virtual void OnAPPPacket(jrtplib::RTCPAPPPacket *apppacket,const jrtplib::RTPTime &receivetime,
	                         const jrtplib::RTPAddress *senderaddress)					;
	
	/** Is called when an unknown RTCP packet type was detected. */
	virtual void OnUnknownPacketType(jrtplib::RTCPPacket *rtcppack,const jrtplib::RTPTime &receivetime,
	                                 const jrtplib::RTPAddress *senderaddress)				;

	/** Is called when an unknown packet format for a known packet type was detected. */
	virtual void OnUnknownPacketFormat(jrtplib::RTCPPacket *rtcppack,const jrtplib::RTPTime &receivetime,
	                                   const jrtplib::RTPAddress *senderaddress)				;

	/** Is called when the SDES NOTE item for source \c srcdat has been timed out. */
	virtual void OnNoteTimeout(jrtplib::RTPSourceData *srcdat)						;

	/** Is called when a BYE packet has been processed for source \c srcdat. */
	virtual void OnBYEPacket(jrtplib::RTPSourceData *srcdat)							;

	/** Is called when an RTCP compound packet has just been sent (useful to inspect outgoing RTCP data). */
	virtual void OnSendRTCPCompoundPacket(jrtplib::RTCPCompoundPacket *pack)					;

#ifdef RTP_SUPPORT_THREAD

    /** Is called when error \c errcode was detected in the poll thread. */
	virtual void OnPollThreadError(int errcode);

	/** Is called each time the poll thread loops.
	 *  Is called each time the poll thread loops. This happens when incoming data was 
	 *  detected or when it's time to send an RTCP compound packet.
	 */
	virtual void OnPollThreadStep();

	/** Is called when the poll thread is started.
	 *  Is called when the poll thread is started. This happens just before entering the
	 *  thread main loop.
	 *  \param stop This can be used to stop the thread immediately without entering the loop.
	*/
	virtual void OnPollThreadStart(bool &stop);

	/** Is called when the poll thread is going to stop.
	 *  Is called when the poll thread is going to stop. This happens just before termitating the thread.
	 */
	virtual void OnPollThreadStop();

#endif // RTP_SUPPORT_THREAD

    virtual void ProcessRTPPacket(const jrtplib::RTPSourceData &srcdat,const jrtplib::RTPPacket &rtppack);

public:
    virtual void Start() {
        CWorker::Start();
        //m_bWorking = true;
    }

    virtual void Stop() {
        CWorker::Stop();
        //m_bWorking = false;
        this->BYEDestroy(jrtplib::RTPTime(0.5), NULL, 0);
    }
//
//    virtual bool IsWorking() const { return m_bWorking; }
//
//private:
//    bool m_bWorking;
};

#endif//AV_RTP_SESSION_H