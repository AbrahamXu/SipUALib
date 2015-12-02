#ifndef _LOOKSIDE_H_
#define _LOOKSIDE_H_

class LookSide
{
public:
    LookSide();
    virtual ~LookSide();
    
    BOOL Open(int nSize, int nMaxCount);
    void Close();
    
    char* New();
    void Delete(char* pData);
    
    int GetSize() const { return m_nSize; }
    int GetLeft() const { return m_nCount; }
    
private:
    CRITICAL_SECTION 	m_cs;
    BOOL				m_bOpen;
    int					m_nSize;
    int					m_nMaxCount;
    int					m_nCount;

    char*				m_pHead;
    char*				m_pEnd;
    char				m_end;
};

#endif//_LOOKSIDE_H_