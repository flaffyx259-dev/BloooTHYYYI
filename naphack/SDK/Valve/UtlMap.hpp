#pragma once
template <typename I>
struct UtlRBTreeLinks_t {
   I m_Left;
   I m_Right;
   I m_Parent;
   I m_Tag;
};

template <typename T>
class CDefLess {
public:
	CDefLess( ) { }
	CDefLess( int i ) { }
	inline bool operator()( const T &lhs, const T &rhs ) const { return ( lhs < rhs ); }
	inline bool operator!( ) const { return false; }
};

// For use with FindClosest
// Move these to a common area if anyone else ever uses them
enum CompareOperands_t {
	k_EEqual = 0x1,
	k_EGreaterThan = 0x2,
	k_ELessThan = 0x4,
	k_EGreaterThanOrEqualTo = k_EGreaterThan | k_EEqual,
	k_ELessThanOrEqualTo = k_ELessThan | k_EEqual,
};


template <typename T, typename I>
struct UtlRBTreeNode_t : UtlRBTreeLinks_t<I> {
   T m_Data;
};

enum NodeColor_t {
	RED = 0,
	BLACK
};

template < class T, class I = unsigned short, typename L = bool ( * )( const T &, const T & ), class M = CUtlMemory< UtlRBTreeNode_t< T, I >, I > >
class CUtlRBTree {
public:

	enum NodeColor_tt {
		REDD = 0,
		BLACKK
	};


	typedef UtlRBTreeNode_t< T, I > Node_t;
	typedef UtlRBTreeLinks_t< I > Links_t;

	// Allocation, deletion
	void  FreeNode( I i );

	// Remove methods
	void     RemoveAt( I i );
	bool     Remove( T const &remove );

	void	Unlink( I elem );

   T& Element( I i ) {
	  return m_Elements.Element( i ).m_Data;
   }

   // Invalid index
   static I InvalidIndex( );

   // Sets the children
   void  SetParent( I i, I parent );
   void  SetLeftChild( I i, I child );
   void  SetRightChild( I i, I child );
   bool  IsLeftChild( I i ) const;

   void  LinkToParent( I i, I parent, bool isLeft );
   void InsertRebalance( I i );

   // Checks if a link is red or black
   bool IsRed( I i ) const;
   bool IsBlack( I i ) const;

   void RotateLeft( I i );
   void RotateRight( I i );

   // Gets at the links
   Links_t const &Links( I i ) const;
   Links_t &Links( I i );

   // Inserts a node into the tree, doesn't copy the data in.
   void FindInsertionPosition( T const &insert, I &parent, bool &leftchild );

   // Insert method (inserts in order)
   // NOTE: the returned 'index' will be valid as long as the element remains in the tree
   //       (other elements being added/removed will not affect it)
   I  Insert( T const &insert );
   void Insert( const T *pArray, int nItems );

   // Allocation method
   I  NewNode( );

   // Insertion, removal
   I  InsertAt( I parent, bool leftchild );

   // Sets/gets node color
   NodeColor_t Color( I i ) const;
   void        SetColor( I i, NodeColor_t c );
  
   // Tests if root or leaf
   bool  IsRoot( I i ) const;
   void RemoveRebalance( I i );
  // void  LinkToParent( I i, I parent, bool isLeft );

   const T& Element( I i ) const {
	  return m_Elements.Element( i ).m_Data;
   }

   I MaxElement( ) const {
	  return I( m_Elements.NumAllocated( ) );
   }

   I LeftChild( I i ) const {
	  return Links( i ).m_Left;
   }

   I RightChild( I i ) const {
	  return Links( i ).m_Right;
   }

   I Parent( I i ) const {
	  return Links( i ).m_Parent;
   }

   bool IsRightChild( I i ) const {
	  return RightChild( Parent( i ) ) == i;
   }

   bool IsValidIndex( I i ) const {
	  if ( i < 0 )
		 return false;

	  if ( i > m_LastAlloc.index )
		 return false;

	  return LeftChild( i ) != i;
   }

   I Find( const T& Search ) const {
	  I Current = m_Root;
	  while ( Current != I( -1 ) ) {
		 if ( m_LessFunc( Search, Element( Current ) ) )
			Current = LeftChild( Current );
		 else if ( m_LessFunc( Element( Current ), Search ) )
			Current = RightChild( Current );
		 else
			break;
	  }

	  return Current;
   }

   I FirstInorder( ) const {
	  I i = m_Root;
	  while ( LeftChild( i ) != I( -1 ) )
		 i = LeftChild( i );

	  return i;
   }

   I NextInorder( I i ) const {
	  if ( RightChild( i ) != I( -1 ) ) {
		 i = RightChild( i );
		 while ( LeftChild( i ) != I( -1 ) )
			i = LeftChild( i );

		 return i;
	  }

	  I p = Parent( i );
	  while ( IsRightChild( i ) ) {
		 i = p;
		 if ( i == I( -1 ) )
			break;

		 p = Parent( i );
	  }

	  return p;
   }

protected:
  /* const UtlRBTreeLinks_t<I>& Links( I i ) const {
	  static UtlRBTreeLinks_t<I> s_Sentinel = { I( -1 ), I( -1 ), I( -1 ), I( 1 ) };
	  return i == I( -1 ) ? s_Sentinel : *reinterpret_cast< const UtlRBTreeLinks_t<I>* >( &m_Elements.Element( i ) );
   }*/

   L m_LessFunc;

   M m_Elements;
   I m_Root;
   I m_NumElements;
   I m_FirstFree;
   typename M::Iterator_t m_LastAlloc;

   UtlRBTreeNode_t<T, I>* m_pElements;
};

template < class T, class I, typename L, class M >
inline typename CUtlRBTree<T, I, L, M>::Links_t const &CUtlRBTree<T, I, L, M>::Links( I i ) const {
	// Sentinel node, makes life easier
	static const Links_t s_Sentinel =
	{
		// Use M::INVALID_INDEX instead of InvalidIndex() so that this is
		// a compile-time constant -- otherwise it is constructed on the first
		// call!
		M::INVALID_INDEX, M::INVALID_INDEX, M::INVALID_INDEX, CUtlRBTree<T, I, L, M>::BLACKK
	};

	return ( i != InvalidIndex( ) ) ? m_Elements[ i ] : s_Sentinel;
}

template < class T, class I, typename L, class M >
inline typename CUtlRBTree<T, I, L, M>::Links_t &CUtlRBTree<T, I, L, M>::Links( I i ) {
	Assert( i != InvalidIndex( ) );
	return m_Elements[ i ];
}

//-----------------------------------------------------------------------------
// inserts a node into the tree
//-----------------------------------------------------------------------------

// Inserts a node into the tree, doesn't copy the data in.
template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::FindInsertionPosition( T const &insert, I &parent, bool &leftchild ) {
	Assert( !!m_LessFunc );

	/* find where node belongs */
	I current = m_Root;
	parent = InvalidIndex( );
	leftchild = false;
	while( current != InvalidIndex( ) ) {
		parent = current;
		if( m_LessFunc( insert, Element( current ) ) ) {
			leftchild = true; current = LeftChild( current );
		}
		else {
			leftchild = false; current = RightChild( current );
		}
	}
}

//-----------------------------------------------------------------------------
// Invalid index
//-----------------------------------------------------------------------------

template < class T, class I, typename L, class M >
inline I CUtlRBTree<T, I, L, M>::InvalidIndex( ) {
	return ( I )M::InvalidIndex( );
}

//-----------------------------------------------------------------------------
// Rotates node i to the left
//-----------------------------------------------------------------------------

template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::RotateLeft( I elem ) {
	I rightchild = RightChild( elem );
	SetRightChild( elem, LeftChild( rightchild ) );
	if( LeftChild( rightchild ) != InvalidIndex( ) )
		SetParent( LeftChild( rightchild ), elem );

	if( rightchild != InvalidIndex( ) )
		SetParent( rightchild, Parent( elem ) );
	if( !IsRoot( elem ) ) {
		if( IsLeftChild( elem ) )
			SetLeftChild( Parent( elem ), rightchild );
		else
			SetRightChild( Parent( elem ), rightchild );
	}
	else
		m_Root = rightchild;

	SetLeftChild( rightchild, elem );
	if( elem != InvalidIndex( ) )
		SetParent( elem, rightchild );
}


//-----------------------------------------------------------------------------
// Rotates node i to the right
//-----------------------------------------------------------------------------

template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::RotateRight( I elem ) {
	I leftchild = LeftChild( elem );
	SetLeftChild( elem, RightChild( leftchild ) );
	if( RightChild( leftchild ) != InvalidIndex( ) )
		SetParent( RightChild( leftchild ), elem );

	if( leftchild != InvalidIndex( ) )
		SetParent( leftchild, Parent( elem ) );
	if( !IsRoot( elem ) ) {
		if( IsRightChild( elem ) )
			SetRightChild( Parent( elem ), leftchild );
		else
			SetLeftChild( Parent( elem ), leftchild );
	}
	else
		m_Root = leftchild;

	SetRightChild( leftchild, elem );
	if( elem != InvalidIndex( ) )
		SetParent( elem, leftchild );
}

template<class T, class I, typename L, class M>
inline NodeColor_t CUtlRBTree<T, I, L, M>::Color( I i ) const {
	return ( NodeColor_t )( i != InvalidIndex( ) ? m_Elements[ i ].m_Tag : BLACK );
}

template<class T, class I, typename L, class M>
inline void CUtlRBTree<T, I, L, M>::SetColor( I i, NodeColor_t c ) {
	Links( i ).m_Tag = ( I )c;
}

template < class T, class I, typename L, class M >
inline	bool CUtlRBTree<T, I, L, M>::IsRoot( I i ) const {
	return i == m_Root;
}

template < class T, class I, typename L, class M >
inline void  CUtlRBTree<T, I, L, M>::SetParent( I i, I parent ) {
	Links( i ).m_Parent = parent;
}

template < class T, class I, typename L, class M >
inline void  CUtlRBTree<T, I, L, M>::SetLeftChild( I i, I child ) {
	Links( i ).m_Left = child;
}

template < class T, class I, typename L, class M >
inline void  CUtlRBTree<T, I, L, M>::SetRightChild( I i, I child ) {
	Links( i ).m_Right = child;
}

template < class T, class I, typename L, class M >
inline	bool CUtlRBTree<T, I, L, M>::IsLeftChild( I i ) const {
	return LeftChild( Parent( i ) ) == i;
}

template < class T, class I, typename L, class M >
inline bool CUtlRBTree<T, I, L, M>::IsRed( I i ) const {
	return Color( i ) == RED;
}

template < class T, class I, typename L, class M >
inline bool CUtlRBTree<T, I, L, M>::IsBlack( I i ) const {
	return Color( i ) == BLACK;
}


template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::RemoveRebalance( I elem ) {
	while( elem != m_Root && IsBlack( elem ) ) {
		I parent = Parent( elem );

		// If elem is the left child of the parent
		if( elem == LeftChild( parent ) ) {
			// Get our sibling
			I sibling = RightChild( parent );
			if( IsRed( sibling ) ) {
				SetColor( sibling, BLACK );
				SetColor( parent, RED );
				RotateLeft( parent );

				// We may have a new parent now
				parent = Parent( elem );
				sibling = RightChild( parent );
			}
			if( ( IsBlack( LeftChild( sibling ) ) ) && ( IsBlack( RightChild( sibling ) ) ) ) {
				if( sibling != InvalidIndex( ) )
					SetColor( sibling, RED );
				elem = parent;
			}
			else {
				if( IsBlack( RightChild( sibling ) ) ) {
					SetColor( LeftChild( sibling ), BLACK );
					SetColor( sibling, RED );
					RotateRight( sibling );

					// rotation may have changed this
					parent = Parent( elem );
					sibling = RightChild( parent );
				}
				SetColor( sibling, Color( parent ) );
				SetColor( parent, BLACK );
				SetColor( RightChild( sibling ), BLACK );
				RotateLeft( parent );
				elem = m_Root;
			}
		}
		else {
			// Elem is the right child of the parent
			I sibling = LeftChild( parent );
			if( IsRed( sibling ) ) {
				SetColor( sibling, BLACK );
				SetColor( parent, RED );
				RotateRight( parent );

				// We may have a new parent now
				parent = Parent( elem );
				sibling = LeftChild( parent );
			}
			if( ( IsBlack( RightChild( sibling ) ) ) && ( IsBlack( LeftChild( sibling ) ) ) ) {
				if( sibling != InvalidIndex( ) )
					SetColor( sibling, RED );
				elem = parent;
			}
			else {
				if( IsBlack( LeftChild( sibling ) ) ) {
					SetColor( RightChild( sibling ), BLACK );
					SetColor( sibling, RED );
					RotateLeft( sibling );

					// rotation may have changed this
					parent = Parent( elem );
					sibling = LeftChild( parent );
				}
				SetColor( sibling, Color( parent ) );
				SetColor( parent, BLACK );
				SetColor( LeftChild( sibling ), BLACK );
				RotateRight( parent );
				elem = m_Root;
			}
		}
	}
	SetColor( elem, BLACK );
}


template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::Unlink( I elem ) {
	if( elem != InvalidIndex( ) ) {
		I x, y;

		if( ( LeftChild( elem ) == InvalidIndex( ) ) ||
			( RightChild( elem ) == InvalidIndex( ) ) ) {
			/* y has a NIL node as a child */
			y = elem;
		}
		else {
			/* find tree successor with a NIL node as a child */
			y = RightChild( elem );
			while( LeftChild( y ) != InvalidIndex( ) )
				y = LeftChild( y );
		}

		/* x is y's only child */
		if( LeftChild( y ) != InvalidIndex( ) )
			x = LeftChild( y );
		else
			x = RightChild( y );

		/* remove y from the parent chain */
		if( x != InvalidIndex( ) )
			SetParent( x, Parent( y ) );
		if( !IsRoot( y ) ) {
			if( IsLeftChild( y ) )
				SetLeftChild( Parent( y ), x );
			else
				SetRightChild( Parent( y ), x );
		}
		else
			m_Root = x;

		// need to store this off now, we'll be resetting y's color
		NodeColor_t ycolor = Color( y );
		if( y != elem ) {
			// Standard implementations copy the data around, we cannot here.
			// Hook in y to link to the same stuff elem used to.
			SetParent( y, Parent( elem ) );
			SetRightChild( y, RightChild( elem ) );
			SetLeftChild( y, LeftChild( elem ) );

			if( !IsRoot( elem ) )
				if( IsLeftChild( elem ) )
					SetLeftChild( Parent( elem ), y );
				else
					SetRightChild( Parent( elem ), y );
			else
				m_Root = y;

			if( LeftChild( y ) != InvalidIndex( ) )
				SetParent( LeftChild( y ), y );
			if( RightChild( y ) != InvalidIndex( ) )
				SetParent( RightChild( y ), y );

			SetColor( y, Color( elem ) );
		}

		if( ( x != InvalidIndex( ) ) && ( ycolor == BLACK ) )
			RemoveRebalance( x );
	}
}

template < class T, class I, typename L, class M >
void  CUtlRBTree<T, I, L, M>::FreeNode( I i ) {
	Assert( IsValidIndex( i ) && ( i != InvalidIndex( ) ) );
	Destruct( &Element( i ) );
	SetLeftChild( i, i ); // indicates it's in not in the tree
	SetRightChild( i, m_FirstFree );
	m_FirstFree = i;
}


//-----------------------------------------------------------------------------
// Delete a node from the tree
//-----------------------------------------------------------------------------

template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::RemoveAt( I elem ) {
	if( elem != InvalidIndex( ) ) {
		Unlink( elem );

		FreeNode( elem );
		--m_NumElements;

		// Assert( IsValid( ) );
	}
}

template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::LinkToParent( I i, I parent, bool isLeft ) {
	Links_t &elem = Links( i );
	elem.m_Parent = parent;
	elem.m_Left = elem.m_Right = InvalidIndex( );
	elem.m_Tag = RED;

	/* insert node in tree */
	if( parent != InvalidIndex( ) ) {
		if( isLeft )
			Links( parent ).m_Left = i;
		else
			Links( parent ).m_Right = i;
	}
	else {
		m_Root = i;
	}

	InsertRebalance( i );
}

template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::InsertRebalance( I elem ) {
	while( !IsRoot( elem ) && ( Color( Parent( elem ) ) == RED ) ) {
		I parent = Parent( elem );
		I grandparent = Parent( parent );

		/* we have a violation */
		if( IsLeftChild( parent ) ) {
			I uncle = RightChild( grandparent );
			if( IsRed( uncle ) ) {
				/* uncle is RED */
				SetColor( parent, BLACK );
				SetColor( uncle, BLACK );
				SetColor( grandparent, RED );
				elem = grandparent;
			}
			else {
				/* uncle is BLACK */
				if( IsRightChild( elem ) ) {
					/* make x a left child, will change parent and grandparent */
					elem = parent;
					RotateLeft( elem );
					parent = Parent( elem );
					grandparent = Parent( parent );
				}
				/* recolor and rotate */
				SetColor( parent, BLACK );
				SetColor( grandparent, RED );
				RotateRight( grandparent );
			}
		}
		else {
			/* mirror image of above code */
			I uncle = LeftChild( grandparent );
			if( IsRed( uncle ) ) {
				/* uncle is RED */
				SetColor( parent, BLACK );
				SetColor( uncle, BLACK );
				SetColor( grandparent, RED );
				elem = grandparent;
			}
			else {
				/* uncle is BLACK */
				if( IsLeftChild( elem ) ) {
					/* make x a right child, will change parent and grandparent */
					elem = parent;
					RotateRight( parent );
					parent = Parent( elem );
					grandparent = Parent( parent );
				}
				/* recolor and rotate */
				SetColor( parent, BLACK );
				SetColor( grandparent, RED );
				RotateLeft( grandparent );
			}
		}
	}
	SetColor( m_Root, BLACK );
}


//-----------------------------------------------------------------------------
// Allocates/ deallocates nodes
//-----------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable:4389) // '==' : signed/unsigned mismatch
template < class T, class I, typename L, class M >
I  CUtlRBTree<T, I, L, M>::NewNode( ) {
	I elem;

	// Nothing in the free list; add.
	if( m_FirstFree == InvalidIndex( ) ) {
		Assert( m_Elements.IsValidIterator( m_LastAlloc ) || m_NumElements == 0 );
		typename M::Iterator_t it = m_Elements.IsValidIterator( m_LastAlloc ) ? m_Elements.Next( m_LastAlloc ) : m_Elements.First( );
		if( !m_Elements.IsValidIterator( it ) ) {
			//MEM_ALLOC_CREDIT_CLASS( );
			m_Elements.Grow( );

			it = m_Elements.IsValidIterator( m_LastAlloc ) ? m_Elements.Next( m_LastAlloc ) : m_Elements.First( );

			Assert( m_Elements.IsValidIterator( it ) );
			if( !m_Elements.IsValidIterator( it ) ) {
			//	Error( "CUtlRBTree overflow!\n" );
			}
		}
		m_LastAlloc = it;
		elem = m_Elements.GetIndex( m_LastAlloc );
		Assert( m_Elements.IsValidIterator( m_LastAlloc ) );
	}
	else {
		elem = m_FirstFree;
		m_FirstFree = RightChild( m_FirstFree );
	}

	Construct( &Element( elem ) );

	return elem;
}
#pragma warning(pop)

//-----------------------------------------------------------------------------
// Insert a node into the tree
//-----------------------------------------------------------------------------

template < class T, class I, typename L, class M >
I CUtlRBTree<T, I, L, M>::InsertAt( I parent, bool leftchild ) {
	I i = NewNode( );
	LinkToParent( i, parent, leftchild );
	++m_NumElements;

	//Assert( IsValid( ) );

	return i;
}

template < class T, class I, typename L, class M >
I CUtlRBTree<T, I, L, M>::Insert( T const &insert ) {
	// use copy constructor to copy it in
	I parent = InvalidIndex( );
	bool leftchild = false;
	FindInsertionPosition( insert, parent, leftchild );
	I newNode = InsertAt( parent, leftchild );
	CopyConstruct( &Element( newNode ), insert );
	return newNode;
}

template < class T, class I, typename L, class M >
void CUtlRBTree<T, I, L, M>::Insert( const T *pArray, int nItems ) {
	while( nItems-- ) {
		Insert( *pArray++ );
	}
}

template < class T, class I, typename L, class M > bool CUtlRBTree<T, I, L, M>::Remove( T const &search ) {
	I node = Find( search );
	if( node != InvalidIndex( ) ) {
		RemoveAt( node );
		return true;
	}
	return false;
}

struct base_utlmap_t {
public:
	// This enum exists so that FOR_EACH_MAP and FOR_EACH_MAP_FAST cannot accidentally
	// be used on a type that is not a CUtlMap. If the code compiles then all is well.
	// The check for IsUtlMap being true should be free.
	// Using an enum rather than a static const bool ensures that this trick works even
	// with optimizations disabled on gcc.
	enum { IsUtlMap = true };
};

template <typename K, typename T, typename I = unsigned short, typename LessFunc_t = bool ( * )( const K &, const K & )>
class CUtlMap : public base_utlmap_t {
public:
	typedef K KeyType_t;
	typedef T ElemType_t;
	typedef I IndexType_t;

	// constructor, destructor
	// Left at growSize = 0, the memory will first allocate 1 element and double in size
	// at each increment.
	// LessFunc_t is required, but may be set after the constructor using SetLessFunc() below
	CUtlMap( int growSize = 0, int initSize = 0, const LessFunc_t &lessfunc = 0 )
		: m_Tree( growSize, initSize, CKeyLess( lessfunc ) ) {
	}

	CUtlMap( LessFunc_t lessfunc )
		: m_Tree( CKeyLess( lessfunc ) ) {
	}

	void EnsureCapacity( int num ) { m_Tree.EnsureCapacity( num ); }

	// gets particular elements
	ElemType_t &Element( IndexType_t i ) { return m_Tree.Element( i ).elem; }
	const ElemType_t &Element( IndexType_t i ) const { return m_Tree.Element( i ).elem; }
	ElemType_t &operator[]( IndexType_t i ) { return m_Tree.Element( i ).elem; }
	const ElemType_t &operator[]( IndexType_t i ) const { return m_Tree.Element( i ).elem; }
	KeyType_t &Key( IndexType_t i ) { return m_Tree.Element( i ).key; }
	const KeyType_t &Key( IndexType_t i ) const { return m_Tree.Element( i ).key; }


	// Num elements
	unsigned int Count( ) const { return m_Tree.Count( ); }

	// Max "size" of the vector
	IndexType_t  MaxElement( ) const { return m_Tree.MaxElement( ); }

	// Checks if a node is valid and in the map
	bool  IsValidIndex( IndexType_t i ) const { return m_Tree.IsValidIndex( i ); }

	// Checks if the map as a whole is valid
	bool  IsValid( ) const { return m_Tree.IsValid( ); }

	// Invalid index
	static IndexType_t InvalidIndex( ) { return CTree::InvalidIndex( ); }

	// Sets the less func
	void SetLessFunc( LessFunc_t func ) {
		m_Tree.SetLessFunc( CKeyLess( func ) );
	}

	// Insert method (inserts in order)
	IndexType_t  Insert( const KeyType_t &key, const ElemType_t &insert ) {
		Node_t node;
		node.key = key;
		node.elem = insert;
		return m_Tree.Insert( node );
	}

	IndexType_t  Insert( const KeyType_t &key ) {
		Node_t node;
		node.key = key;
		return m_Tree.Insert( node );
	}

	// API to macth src2 for Panormama
	// Note in src2 straight Insert() calls will assert on duplicates
	// Chosing not to take that change until discussed further 

	IndexType_t  InsertWithDupes( const KeyType_t &key, const ElemType_t &insert ) {
		Node_t node;
		node.key = key;
		node.elem = insert;
		return m_Tree.Insert( node );
	}

	IndexType_t  InsertWithDupes( const KeyType_t &key ) {
		Node_t node;
		node.key = key;
		return m_Tree.Insert( node );
	}


	bool HasElement( const KeyType_t &key ) const {
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.HasElement( dummyNode );
	}


	// Find method
	IndexType_t  Find( const KeyType_t &key ) const {
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.Find( dummyNode );
	}

	// FindFirst method
	// This finds the first inorder occurrence of key
	IndexType_t  FindFirst( const KeyType_t &key ) const {
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.FindFirst( dummyNode );
	}


	const ElemType_t &FindElement( const KeyType_t &key, const ElemType_t &defaultValue ) const {
		IndexType_t i = Find( key );
		if( i == InvalidIndex( ) )
			return defaultValue;
		return Element( i );
	}


	// First element >= key
	IndexType_t  FindClosest( const KeyType_t &key, CompareOperands_t eFindCriteria ) const {
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.FindClosest( dummyNode, eFindCriteria );
	}

	// Remove methods
	void     RemoveAt( IndexType_t i ) { m_Tree.RemoveAt( i ); }
	bool     Remove( const KeyType_t &key ) {
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.Remove( dummyNode );
	}

	void     RemoveAll( ) { m_Tree.RemoveAll( ); }
	void     Purge( ) { m_Tree.Purge( ); }

	// Purges the list and calls delete on each element in it.
	void PurgeAndDeleteElements( );

	// Iteration
	IndexType_t  FirstInorder( ) const { return m_Tree.FirstInorder( ); }
	IndexType_t  NextInorder( IndexType_t i ) const { return m_Tree.NextInorder( i ); }
	IndexType_t  PrevInorder( IndexType_t i ) const { return m_Tree.PrevInorder( i ); }
	IndexType_t  LastInorder( ) const { return m_Tree.LastInorder( ); }

	// API Matching src2 for Panorama
	IndexType_t  NextInorderSameKey( IndexType_t i ) const {
		IndexType_t iNext = NextInorder( i );
		if( !IsValidIndex( iNext ) )
			return InvalidIndex( );
		if( Key( iNext ) != Key( i ) )
			return InvalidIndex( );
		return iNext;
	}

	// If you change the search key, this can be used to reinsert the 
	// element into the map.
	void	Reinsert( const KeyType_t &key, IndexType_t i ) {
		m_Tree[ i ].key = key;
		m_Tree.Reinsert( i );
	}

	IndexType_t InsertOrReplace( const KeyType_t &key, const ElemType_t &insert ) {
		IndexType_t i = Find( key );
		if( i != InvalidIndex( ) ) {
			Element( i ) = insert;
			return i;
		}

		return Insert( key, insert );
	}

	void Swap( CUtlMap< K, T, I > &that ) {
		m_Tree.Swap( that.m_Tree );
	}


	struct Node_t {
		Node_t( ) {
		}

		Node_t( const Node_t &from )
			: key( from.key ),
			elem( from.elem ) {
		}

		KeyType_t	key;
		ElemType_t	elem;
	};

	class CKeyLess {
	public:
		CKeyLess( const LessFunc_t &lessFunc ) : m_LessFunc( lessFunc ) { }

		bool operator!( ) const {
			return !m_LessFunc;
		}

		bool operator()( const Node_t &left, const Node_t &right ) const {
			return m_LessFunc( left.key, right.key );
		}

		LessFunc_t m_LessFunc;
	};

	typedef CUtlRBTree<Node_t, I, CKeyLess> CTree;

	CTree *AccessTree( ) { return &m_Tree; }

protected:
	CTree 	   m_Tree;
};

//-----------------------------------------------------------------------------

// Purges the list and calls delete on each element in it.
template< typename K, typename T, typename I, typename LessFunc_t >
inline void CUtlMap<K, T, I, LessFunc_t>::PurgeAndDeleteElements( ) {
	for( I i = 0; i < MaxElement( ); ++i ) {
		if( !IsValidIndex( i ) )
			continue;

		delete Element( i );
	}

	Purge( );
}
