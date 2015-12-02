#pragma once
class CWorker
{
public:
    CWorker(void);
    virtual ~CWorker(void);

    virtual void Start() {
        ASSERT(!m_bWorking);
        m_bWorking = true;
    }
    virtual void Stop() {
        ASSERT(m_bWorking);
        m_bWorking = false;
    }
    bool IsWorking() const { return m_bWorking; }
    //void setWorking(bool working) { m_bWorking = working; }

private:
    bool m_bWorking;
};

