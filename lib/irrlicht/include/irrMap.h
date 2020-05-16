// Copyright (C) 2006-2012 by Kat'Oun
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_MAP_H_INCLUDED__
#define __IRR_MAP_H_INCLUDED__

#include "irrTypes.h"
#include "irrMath.h"

namespace irr
{
namespace core
{

//! map template for associative arrays using a red-black tree
template <class KeyType, class ValueType>
class map
{
	//! red/black tree for map
	template <class KeyTypeRB, class ValueTypeRB>
	class RBTree
	{
	public:

		RBTree(const KeyTypeRB& k, const ValueTypeRB& v)
			: LeftChild(0), RightChild(0), Parent(0), Key(k),
				Value(v), IsRed(true) {}

		void setLeftChild(RBTree* p)
		{
			LeftChild=p;
			if (p)
				p->setParent(this);
		}

		void setRightChild(RBTree* p)
		{
			RightChild=p;
			if (p)
				p->setParent(this);
		}

		void setParent(RBTree* p)		{ Parent=p; }

		void setValue(const ValueTypeRB& v)	{ Value = v; }

		void setRed()			{ IsRed = true; }
		void setBlack()			{ IsRed = false; }

		RBTree* getLeftChild() const	{ return LeftChild; }
		RBTree* getRightChild() const	{ return RightChild; }
		RBTree* getParent() const		{ return Parent; }

		const ValueTypeRB& getValue() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return Value;
		}

		ValueTypeRB& getValue()
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return Value;
		}

		const KeyTypeRB& getKey() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return Key;
		}

		bool isRoot() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return Parent==0;
		}

		bool isLeftChild() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return (Parent != 0) && (Parent->getLeftChild()==this);
		}

		bool isRightChild() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return (Parent!=0) && (Parent->getRightChild()==this);
		}

		bool isLeaf() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return (LeftChild==0) && (RightChild==0);
		}

		unsigned int getLevel() const
		{
			if (isRoot())
				return 1;
			else
				return getParent()->getLevel() + 1;
		}


		bool isRed() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return IsRed;
		}

		bool isBlack() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return !IsRed;
		}

	private:
		RBTree();

		RBTree*		LeftChild;
		RBTree*		RightChild;

		RBTree*		Parent;

		KeyTypeRB	Key;
		ValueTypeRB	Value;

		bool IsRed;
	}; // RBTree

	public:

	typedef RBTree<KeyType,ValueType> Node;
	// We need the forwad declaration for the friend declaration
	class ConstIterator;

	//! Normal Iterator
	class Iterator
	{
		friend class ConstIterator;
	public:

		Iterator() : Root(0), Cur(0) {}

		// Constructor(Node*)
		Iterator(Node* root) : Root(root)
		{
			reset();
		}

		// Copy constructor
		Iterator(const Iterator& src) : Root(src.Root), Cur(src.Cur) {}

		void reset(bool atLowest=true)
		{
			if (atLowest)
				Cur = getMin(Root);
			else
				Cur = getMax(Root);
		}

		bool atEnd() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return Cur==0;
		}

		Node* getNode() const
		{
			return Cur;
		}

		Iterator& operator=(const Iterator& src)
		{
			Root = src.Root;
			Cur = src.Cur;
			return (*this);
		}

		void operator++(int)
		{
			inc();
		}

		void operator--(int)
		{
			dec();
		}

		Node* operator->()
		{
			return getNode();
		}

		Node& operator*()
		{
			_IRR_DEBUG_BREAK_IF(atEnd()) // access violation

			return *Cur;
		}

	private:

		Node* getMin(Node* n) const
		{
			while(n && n->getLeftChild())
				n = n->getLeftChild();
			return n;
		}

		Node* getMax(Node* n) const
		{
			while(n && n->getRightChild())
				n = n->getRightChild();
			return n;
		}

		void inc()
		{
			// Already at end?
			if (Cur==0)
				return;

			if (Cur->getRightChild())
			{
				// If current node has a right child, the next higher node is the
				// node with lowest key beneath the right child.
				Cur = getMin(Cur->getRightChild());
			}
			else if (Cur->isLeftChild())
			{
				// No right child? Well if current node is a left child then
				// the next higher node is the parent
				Cur = Cur->getParent();
			}
			else
			{
				// Current node neither is left child nor has a right child.
				// Ie it is either right child or root
				// The next higher node is the parent of the first non-right
				// child (ie either a left child or the root) up in the
				// hierarchy. Root's parent is 0.
				while(Cur->isRightChild())
					Cur = Cur->getParent();
				Cur = Cur->getParent();
			}
		}

		void dec()
		{
			// Already at end?
			if (Cur==0)
				return;

			if (Cur->getLeftChild())
			{
				// If current node has a left child, the next lower node is the
				// node with highest key beneath the left child.
				Cur = getMax(Cur->getLeftChild());
			}
			else if (Cur->isRightChild())
			{
				// No left child? Well if current node is a right child then
				// the next lower node is the parent
				Cur = Cur->getParent();
			}
			else
			{
				// Current node neither is right child nor has a left child.
				// Ie it is either left child or root
				// The next higher node is the parent of the first non-left
				// child (ie either a right child or the root) up in the
				// hierarchy. Root's parent is 0.

				while(Cur->isLeftChild())
					Cur = Cur->getParent();
				Cur = Cur->getParent();
			}
		}

		Node* Root;
		Node* Cur;
	}; // Iterator

	//! Const Iterator
	class ConstIterator
	{
		friend class Iterator;
	public:

		ConstIterator() : Root(0), Cur(0) {}

		// Constructor(Node*)
		ConstIterator(const Node* root) : Root(root)
		{
			reset();
		}

		// Copy constructor
		ConstIterator(const ConstIterator& src) : Root(src.Root), Cur(src.Cur) {}
		ConstIterator(const Iterator& src) : Root(src.Root), Cur(src.Cur) {}

		void reset(bool atLowest=true)
		{
			if (atLowest)
				Cur = getMin(Root);
			else
				Cur = getMax(Root);
		}

		bool atEnd() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return Cur==0;
		}

		const Node* getNode() const
		{
			return Cur;
		}

		ConstIterator& operator=(const ConstIterator& src)
		{
			Root = src.Root;
			Cur = src.Cur;
			return (*this);
		}

		void operator++(int)
		{
			inc();
		}

		void operator--(int)
		{
			dec();
		}

		const Node* operator->()
		{
			return getNode();
		}

		const Node& operator*()
		{
			_IRR_DEBUG_BREAK_IF(atEnd()) // access violation

			return *Cur;
		}

	private:

		const Node* getMin(const Node* n) const
		{
			while(n && n->getLeftChild())
				n = n->getLeftChild();
			return n;
		}

		const Node* getMax(const Node* n) const
		{
			while(n && n->getRightChild())
				n = n->getRightChild();
			return n;
		}

		void inc()
		{
			// Already at end?
			if (Cur==0)
				return;

			if (Cur->getRightChild())
			{
				// If current node has a right child, the next higher node is the
				// node with lowest key beneath the right child.
				Cur = getMin(Cur->getRightChild());
			}
			else if (Cur->isLeftChild())
			{
				// No right child? Well if current node is a left child then
				// the next higher node is the parent
				Cur = Cur->getParent();
			}
			else
			{
				// Current node neither is left child nor has a right child.
				// Ie it is either right child or root
				// The next higher node is the parent of the first non-right
				// child (ie either a left child or the root) up in the
				// hierarchy. Root's parent is 0.
				while(Cur->isRightChild())
					Cur = Cur->getParent();
				Cur = Cur->getParent();
			}
		}

		void dec()
		{
			// Already at end?
			if (Cur==0)
				return;

			if (Cur->getLeftChild())
			{
				// If current node has a left child, the next lower node is the
				// node with highest key beneath the left child.
				Cur = getMax(Cur->getLeftChild());
			}
			else if (Cur->isRightChild())
			{
				// No left child? Well if current node is a right child then
				// the next lower node is the parent
				Cur = Cur->getParent();
			}
			else
			{
				// Current node neither is right child nor has a left child.
				// Ie it is either left child or root
				// The next higher node is the parent of the first non-left
				// child (ie either a right child or the root) up in the
				// hierarchy. Root's parent is 0.

				while(Cur->isLeftChild())
					Cur = Cur->getParent();
				Cur = Cur->getParent();
			}
		}

		const Node* Root;
		const Node* Cur;
	}; // ConstIterator


	//! Parent First Iterator.
	/** Traverses the tree from top to bottom. Typical usage is
	when storing the tree structure, because when reading it
	later (and inserting elements) the tree structure will
	be the same. */
	class ParentFirstIterator
	{
	public:

	ParentFirstIterator() : Root(0), Cur(0)	{}

	explicit ParentFirstIterator(Node* root) : Root(root), Cur(0)
	{
		reset();
	}

	void reset()
	{
		Cur = Root;
	}

	bool atEnd() const
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return Cur==0;
	}

	Node* getNode()
	{
		return Cur;
	}

	ParentFirstIterator& operator=(const ParentFirstIterator& src)
	{
		Root = src.Root;
		Cur = src.Cur;
		return (*this);
	}

	void operator++(int)
	{
		inc();
	}

	Node* operator -> ()
	{
		return getNode();
	}

	Node& operator* ()
	{
		_IRR_DEBUG_BREAK_IF(atEnd()) // access violation

		return *getNode();
	}

	private:

	void inc()
	{
		// Already at end?
		if (Cur==0)
			return;

		// First we try down to the left
		if (Cur->getLeftChild())
		{
			Cur = Cur->getLeftChild();
		}
		else if (Cur->getRightChild())
		{
			// No left child? The we go down to the right.
			Cur = Cur->getRightChild();
		}
		else
		{
			// No children? Move up in the hierarcy until
			// we either reach 0 (and are finished) or
			// find a right uncle.
			while (Cur!=0)
			{
				// But if parent is left child and has a right "uncle" the parent
				// has already been processed but the uncle hasn't. Move to
				// the uncle.
				if (Cur->isLeftChild() && Cur->getParent()->getRightChild())
				{
					Cur = Cur->getParent()->getRightChild();
					return;
				}
				Cur = Cur->getParent();
			}
		}
	}

	Node* Root;
	Node* Cur;

	}; // ParentFirstIterator


	//! Parent Last Iterator
	/** Traverse the tree from bottom to top.
	Typical usage is when deleting all elements in the tree
	because you must delete the children before you delete
	their parent. */
	class ParentLastIterator
	{
	public:

		ParentLastIterator() : Root(0), Cur(0) {}

		explicit ParentLastIterator(Node* root) : Root(root), Cur(0)
		{
			reset();
		}

		void reset()
		{
			Cur = getMin(Root);
		}

		bool atEnd() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return Cur==0;
		}

		Node* getNode()
		{
			return Cur;
		}

		ParentLastIterator& operator=(const ParentLastIterator& src)
		{
			Root = src.Root;
			Cur = src.Cur;
			return (*this);
		}

		void operator++(int)
		{
			inc();
		}

		Node* operator -> ()
		{
			return getNode();
		}

		Node& operator* ()
		{
			_IRR_DEBUG_BREAK_IF(atEnd()) // access violation

			return *getNode();
		}
	private:

		Node* getMin(Node* n)
		{
			while(n!=0 && (n->getLeftChild()!=0 || n->getRightChild()!=0))
			{
				if (n->getLeftChild())
					n = n->getLeftChild();
				else
					n = n->getRightChild();
			}
			return n;
		}

		void inc()
		{
			// Already at end?
			if (Cur==0)
				return;

			// Note: Starting point is the node as far down to the left as possible.

			// If current node has an uncle to the right, go to the
			// node as far down to the left from the uncle as possible
			// else just go up a level to the parent.
			if (Cur->isLeftChild() && Cur->getParent()->getRightChild())
			{
				Cur = getMin(Cur->getParent()->getRightChild());
			}
			else
				Cur = Cur->getParent();
		}

		Node* Root;
		Node* Cur;
	}; // ParentLastIterator


	// AccessClass is a temporary class used with the [] operator.
	// It makes it possible to have different behavior in situations like:
	// myTree["Foo"] = 32;
	// If "Foo" already exists update its value else insert a new element.
	// int i = myTree["Foo"]
	// If "Foo" exists return its value.
	class AccessClass
	{
		// Let map be the only one who can instantiate this class.
		friend class map<KeyType, ValueType>;

	public:

		// Assignment operator. Handles the myTree["Foo"] = 32; situation
		void operator=(const ValueType& value)
		{
			// Just use the Set method, it handles already exist/not exist situation
			Tree.set(Key,value);
		}

		// ValueType operator
		operator ValueType()
		{
			Node* node = Tree.find(Key);

			// Not found
			_IRR_DEBUG_BREAK_IF(node==0) // access violation

			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return node->getValue();
		}

	private:

		AccessClass(map& tree, const KeyType& key) : Tree(tree), Key(key) {}

		AccessClass();

		map& Tree;
		const KeyType& Key;
	}; // AccessClass


	// Constructor.
	map() : Root(0), Size(0) {}

	// Destructor
	~map()
	{
		clear();
	}

	//------------------------------
	// Public Commands
	//------------------------------

	//! Inserts a new node into the tree
	/** \param keyNew: the index for this value
	\param v: the value to insert
	\return True if successful, false if it fails (already exists) */
	bool insert(const KeyType& keyNew, const ValueType& v)
	{
		// First insert node the "usual" way (no fancy balance logic yet)
		Node* newNode = new Node(keyNew,v);
		if (!insert(newNode))
		{
			delete newNode;
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return false;
		}

		// Then attend a balancing party
		while (!newNode->isRoot() && (newNode->getParent()->isRed()))
		{
			if (newNode->getParent()->isLeftChild())
			{
				// If newNode is a left child, get its right 'uncle'
				Node* newNodesUncle = newNode->getParent()->getParent()->getRightChild();
				if ( newNodesUncle!=0 && newNodesUncle->isRed())
				{
					// case 1 - change the colors
					newNode->getParent()->setBlack();
					newNodesUncle->setBlack();
					newNode->getParent()->getParent()->setRed();
					// Move newNode up the tree
					newNode = newNode->getParent()->getParent();
				}
				else
				{
					// newNodesUncle is a black node
					if ( newNode->isRightChild())
					{
						// and newNode is to the right
						// case 2 - move newNode up and rotate
						newNode = newNode->getParent();
						rotateLeft(newNode);
					}
					// case 3
					newNode->getParent()->setBlack();
					newNode->getParent()->getParent()->setRed();
					rotateRight(newNode->getParent()->getParent());
				}
			}
			else
			{
				// If newNode is a right child, get its left 'uncle'
				Node* newNodesUncle = newNode->getParent()->getParent()->getLeftChild();
				if ( newNodesUncle!=0 && newNodesUncle->isRed())
				{
					// case 1 - change the colors
					newNode->getParent()->setBlack();
					newNodesUncle->setBlack();
					newNode->getParent()->getParent()->setRed();
					// Move newNode up the tree
					newNode = newNode->getParent()->getParent();
				}
				else
				{
					// newNodesUncle is a black node
					if (newNode->isLeftChild())
					{
						// and newNode is to the left
						// case 2 - move newNode up and rotate
						newNode = newNode->getParent();
						rotateRight(newNode);
					}
					// case 3
					newNode->getParent()->setBlack();
					newNode->getParent()->getParent()->setRed();
					rotateLeft(newNode->getParent()->getParent());
				}

			}
		}
		// Color the root black
		Root->setBlack();
		return true;
	}

	//! Replaces the value if the key already exists, otherwise inserts a new element.
	/** \param k The index for this value
	\param v The new value of */
	void set(const KeyType& k, const ValueType& v)
	{
		Node* p = find(k);
		if (p)
			p->setValue(v);
		else
			insert(k,v);
	}

	//! Removes a node from the tree and returns it.
	/** The returned node must be deleted by the user
	\param k the key to remove
	\return A pointer to the node, or 0 if not found */
	Node* delink(const KeyType& k)
	{
		Node* p = find(k);
		if (p == 0)
			return 0;

		// Rotate p down to the left until it has no right child, will get there
		// sooner or later.
		while(p->getRightChild())
		{
			// "Pull up my right child and let it knock me down to the left"
			rotateLeft(p);
		}
		// p now has no right child but might have a left child
		Node* left = p->getLeftChild();

		// Let p's parent point to p's child instead of point to p
		if (p->isLeftChild())
			p->getParent()->setLeftChild(left);

		else if (p->isRightChild())
			p->getParent()->setRightChild(left);

		else
		{
			// p has no parent => p is the root.
			// Let the left child be the new root.
			setRoot(left);
		}

		// p is now gone from the tree in the sense that
		// no one is pointing at it, so return it.

		--Size;
		return p;
	}

	//! Removes a node from the tree and deletes it.
	/** \return True if the node was found and deleted */
	bool remove(const KeyType& k)
	{
		Node* p = find(k);
		return remove(p);
	}

	//! Removes a node from the tree and deletes it.
	/** \return True if the node was found and deleted */
	bool remove(Node* p)
	{
		if (p == 0)
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return false;
		}

		// Rotate p down to the left until it has no right child, will get there
		// sooner or later.
		while(p->getRightChild())
		{
			// "Pull up my right child and let it knock me down to the left"
			rotateLeft(p);
		}
		// p now has no right child but might have a left child
		Node* left = p->getLeftChild();

		// Let p's parent point to p's child instead of point to p
		if (p->isLeftChild())
			p->getParent()->setLeftChild(left);

		else if (p->isRightChild())
			p->getParent()->setRightChild(left);

		else
		{
			// p has no parent => p is the root.
			// Let the left child be the new root.
			setRoot(left);
		}

		// p is now gone from the tree in the sense that
		// no one is pointing at it. Let's get rid of it.
		delete p;

		--Size;
		return true;
	}

	//! Clear the entire tree
	void clear()
	{
		ParentLastIterator i(getParentLastIterator());

		while(!i.atEnd())
		{
			Node* p = i.getNode();
			i++; // Increment it before it is deleted
				// else iterator will get quite confused.
			delete p;
		}
		Root = 0;
		Size= 0;
	}

	//! Is the tree empty?
	//! \return Returns true if empty, false if not
	bool empty() const
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return Root == 0;
	}

	//! \deprecated Use empty() instead. This method may be removed by Irrlicht 1.9
	_IRR_DEPRECATED_ bool isEmpty() const
	{
		return empty();
	}

	//! Search for a node with the specified key.
	//! \param keyToFind: The key to find
	//! \return Returns 0 if node couldn't be found.
	Node* find(const KeyType& keyToFind) const
	{
		Node* pNode = Root;

		while(pNode!=0)
		{
			const KeyType& key=pNode->getKey();

			if (keyToFind == key)
				return pNode;
			else if (keyToFind < key)
				pNode = pNode->getLeftChild();
			else //keyToFind > key
				pNode = pNode->getRightChild();
		}

		return 0;
	}

	//! Gets the root element.
	//! \return Returns a pointer to the root node, or
	//! 0 if the tree is empty.
	Node* getRoot() const
	{
		return Root;
	}

	//! Returns the number of nodes in the tree.
	u32 size() const
	{
		return Size;
	}

	//! Swap the content of this map container with the content of another map
	/** Afterwards this object will contain the content of the other object and the other
	object will contain the content of this object. Iterators will afterwards be valid for
	the swapped object.
	\param other Swap content with this object	*/
	void swap(map<KeyType, ValueType>& other)
	{
		core::swap(Root, other.Root);
		core::swap(Size, other.Size);
	}

	//------------------------------
	// Public Iterators
	//------------------------------

	//! Returns an iterator
	Iterator getIterator() const
	{
		Iterator it(getRoot());
		return it;
	}

	//! Returns a Constiterator
	ConstIterator getConstIterator() const
	{
		Iterator it(getRoot());
		return it;
	}

	//! Returns a ParentFirstIterator.
	//! Traverses the tree from top to bottom. Typical usage is
	//! when storing the tree structure, because when reading it
	//! later (and inserting elements) the tree structure will
	//! be the same.
	ParentFirstIterator getParentFirstIterator() const
	{
		ParentFirstIterator it(getRoot());
		return it;
	}

	//! Returns a ParentLastIterator to traverse the tree from
	//! bottom to top.
	//! Typical usage is when deleting all elements in the tree
	//! because you must delete the children before you delete
	//! their parent.
	ParentLastIterator getParentLastIterator() const
	{
		ParentLastIterator it(getRoot());
		return it;
	}

	//------------------------------
	// Public Operators
	//------------------------------

	//! operator [] for access to elements
	/** for example myMap["key"] */
	AccessClass operator[](const KeyType& k)
	{
		return AccessClass(*this, k);
	}
	private:

	//------------------------------
	// Disabled methods
	//------------------------------
	// Copy constructor and assignment operator deliberately
	// defined but not implemented. The tree should never be
	// copied, pass along references to it instead.
	explicit map(const map& src);
	map& operator = (const map& src);

	//! Set node as new root.
	/** The node will be set to black, otherwise core dumps may arise
	(patch provided by rogerborg).
	\param newRoot Node which will be the new root
	*/
	void setRoot(Node* newRoot)
	{
		Root = newRoot;
		if (Root != 0)
		{
			Root->setParent(0);
			Root->setBlack();
		}
	}

	//! Insert a node into the tree without using any fancy balancing logic.
	/** \return false if that key already exist in the tree. */
	bool insert(Node* newNode)
	{
		bool result=true; // Assume success

		if (Root==0)
		{
			setRoot(newNode);
			Size = 1;
		}
		else
		{
			Node* pNode = Root;
			const KeyType& keyNew = newNode->getKey();
			while (pNode)
			{
				const KeyType& key=pNode->getKey();

				if (keyNew == key)
				{
					result = false;
					pNode = 0;
				}
				else if (keyNew < key)
				{
					if (pNode->getLeftChild() == 0)
					{
						pNode->setLeftChild(newNode);
						pNode = 0;
					}
					else
						pNode = pNode->getLeftChild();
				}
				else // keyNew > key
				{
					if (pNode->getRightChild()==0)
					{
						pNode->setRightChild(newNode);
						pNode = 0;
					}
					else
					{
						pNode = pNode->getRightChild();
					}
				}
			}

			if (result)
				++Size;
		}

		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return result;
	}

	//! Rotate left.
	//! Pull up node's right child and let it knock node down to the left
	void rotateLeft(Node* p)
	{
		Node* right = p->getRightChild();

		p->setRightChild(right->getLeftChild());

		if (p->isLeftChild())
			p->getParent()->setLeftChild(right);
		else if (p->isRightChild())
			p->getParent()->setRightChild(right);
		else
			setRoot(right);

		right->setLeftChild(p);
	}

	//! Rotate right.
	//! Pull up node's left child and let it knock node down to the right
	void rotateRight(Node* p)
	{
		Node* left = p->getLeftChild();

		p->setLeftChild(left->getRightChild());

		if (p->isLeftChild())
			p->getParent()->setLeftChild(left);
		else if (p->isRightChild())
			p->getParent()->setRightChild(left);
		else
			setRoot(left);

		left->setRightChild(p);
	}

	//------------------------------
	// Private Members
	//------------------------------
	Node* Root; // The top node. 0 if empty.
	u32 Size; // Number of nodes in the tree
};

} // end namespace core
} // end namespace irr

#endif // __IRR_MAP_H_INCLUDED__

