// ----------------------------------------------------------------------- //
//
// MODULE  : LinkList.h
//
// PURPOSE : TLinkList - a singly linked list of pointers
//
//           Primal Hunt (Third Law). No source shipped: the list every new
//           Primal Hunt class keeps (the leash groups, nodes and volumes,
//           the leash faces' vertices and edge planes). Written from
//           PHObject.lto; the template's own name, and every member name,
//           are ours.
//
//           The list is its own sentinel: m_Head's m_pNext is the first
//           node and the last node's m_pNext is &m_Head (an empty list's
//           m_Head points at itself). A list owns its items: the destructor
//           and DeleteAll() delete them, RemoveAll() only frees the nodes.
//
// ----------------------------------------------------------------------- //

#ifndef __LINK_LIST_H__
#define __LINK_LIST_H__

#ifdef PRIMAL_HUNT

template <class T>
class TLinkNode
{
	public :

		TLinkNode() { m_pNext = LTNULL; }
		~TLinkNode() {}

		T				m_Data;
		TLinkNode<T>*	m_pNext;
};

template <class T>
class TLinkList
{
	public :

		typedef TLinkNode<T> Node;

		TLinkList()
		{
			m_Head.m_Data = 0;
			m_Head.m_pNext = &m_Head;
			m_pTail = &m_Head;
			m_nCount = 0;
		}

		~TLinkList() { DeleteAll(); }

		Node*	GetHead()				{ return m_Head.m_pNext; }
		Node*	GetEnd()				{ return &m_Head; }
		int		GetCount() const		{ return m_nCount; }

		// e.g. 0x1014bae0
		Node* AddHead(T data)
		{
			Node* pNode = new Node;
			if (!pNode) return LTNULL;

			pNode->m_Data = data;
			pNode->m_pNext = m_Head.m_pNext;
			m_Head.m_pNext = pNode;
			if (pNode->m_pNext == &m_Head)
			{
				m_pTail = pNode;
			}

			m_nCount++;
			return pNode;
		}

		// e.g. 0x10151710
		Node* AddTail(T data)
		{
			Node* pNode = new Node;
			if (!pNode) return LTNULL;

			pNode->m_Data = data;
			pNode->m_pNext = &m_Head;
			m_pTail->m_pNext = pNode;
			m_pTail = pNode;

			m_nCount++;
			return pNode;
		}

		// Unlinks and frees pNode, keeping its item.
		LTBOOL Remove(Node* pNode)
		{
			Node* pPrev = &m_Head;
			Node* pCur = m_Head.m_pNext;
			while (pCur != &m_Head && pCur != pNode)
			{
				pPrev = pCur;
				pCur = pCur->m_pNext;
			}

			if (pCur == &m_Head) return LTFALSE;

			pPrev->m_pNext = pNode->m_pNext;
			if (m_pTail == pNode)
			{
				m_pTail = pPrev;
			}

			delete pNode;
			m_nCount--;
			return LTTRUE;
		}

		// Unlinks and frees pNode, and deletes its item.
		void Delete(Node* pNode)
		{
			if (!pNode || pNode == &m_Head) return;

			T data = pNode->m_Data;
			if (!Remove(pNode)) return;

			delete data;

			// RETAIL BUG: Remove() has already counted the node out, so
			// every Delete() leaves m_nCount one too low (0x1015275a).
			m_nCount--;
		}

		// e.g. 0x10151870
		void RemoveAll()
		{
			while (m_Head.m_pNext != &m_Head)
			{
				Node* pNode = m_Head.m_pNext;
				Node* pNext = pNode->m_pNext;
				delete pNode;
				m_Head.m_pNext = pNext;
			}

			m_Head.m_pNext = &m_Head;
			m_pTail = &m_Head;
			m_nCount = 0;
		}

		// e.g. 0x101515c0's body
		void DeleteAll()
		{
			while (m_Head.m_pNext != &m_Head)
			{
				Node* pNode = m_Head.m_pNext;
				Node* pNext = pNode->m_pNext;
				T data = pNode->m_Data;
				delete pNode;
				delete data;
				m_Head.m_pNext = pNext;
			}

			m_Head.m_pNext = &m_Head;
			m_pTail = &m_Head;
			m_nCount = 0;
		}

	protected :

		Node	m_Head;
		Node*	m_pTail;
		int		m_nCount;
};

#endif // PRIMAL_HUNT

#endif // __LINK_LIST_H__
