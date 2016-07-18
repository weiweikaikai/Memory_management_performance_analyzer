#pragma once
#include <vector>
#include <string>

template<class K, class V>
struct HashTableNode
{
	K _key;
	V _value;
	HashTableNode<K, V>* _next;

	HashTableNode(const K& key, const V& value)
		:_key(key)
		,_value(value)
		,_next(NULL)
	{}
};

template<class K>
struct DefaultHashFuner
{
	size_t operator() (const K& key)
	{
		return key;
	}
};

template<>
struct DefaultHashFuner<string>
{
	static size_t BKDRHash(const char * str)
	{
		unsigned int seed = 131; // 31 131 1313 13131 131313
		unsigned int hash = 0;
		while (*str )
		{
			hash = hash * seed + (* str++);
		}
		return (hash & 0x7FFFFFFF);
	}
	size_t operator() (const string& key)
	{
		return BKDRHash(key.c_str());
	}
};

template<class K, class V, class HashFun = DefaultHashFuner<K> >
class HashTableBucket
{
	typedef HashTableNode<K,V> KVNode;

public:
	HashTableBucket()
		:_size(0)
	{}

	HashTableBucket(size_t size)
		:_size(0)
	{
		_tables.resize(size);
	}

	void Swap(HashTableBucket<K,V,HashFun>& ht)
	{
		_tables.swap(ht._tables);
		swap(_size, ht._size);
	}

	HashTableBucket(const HashTableBucket<K,V,HashFun>& ht)
		:_size(0)
	{
		HashTableBucket<K, V, HashFun> newTables(ht._tables.size());
		for (size_t i = 0; i < ht._tables.size(); ++i)
		{
			KVNode* cur = ht._tables[i];
			KVNode* & head = newTables._tables[i];

			while (cur)
			{
				//newTables.Insert(cur->_key, cur->_value);

				// 头插
				KVNode* tmp = new KVNode(cur->_key, cur->_value);
				tmp->_next = head;
				head = tmp;

				cur = cur->_next;
			}
		}
		this->Swap(newTables);
	}

	//HashTableBucket<K,V,HashFun>& operator=(HashTableBucket<K,V,HashFun> ht)
	//{
	//	this->Swap(ht);

	//	return *this;
	//}

	HashTableBucket<K,V,HashFun>& operator=(const HashTableBucket<K,V,HashFun>& ht)
	{
		if (&ht != this)
		{
			HashTableBucket<K,V,HashFun> tmp(ht);
			this->Swap(tmp);
		}

		return *this;
	}

	~HashTableBucket()
	{
		for (size_t i = 0; i < _tables.size() ; ++i)
		{
			KVNode* cur = _tables[i];
			while (cur)
			{
				KVNode* del = cur;
				cur = cur->_next;

				delete del;
			}

			_tables[i] = NULL;
		}

		_size = 0;
	}

	// operator==
	// Hash转换处理
	bool Insert(const K& key, const V& value)
	{
		_CheckCapacity();

		size_t index = HashFunc(key, _tables.size());
	
		// 查找Key是否存在
		KVNode* cur = _tables[index];
		while (cur)
		{
			if (cur->_key == key)
				return false;

			cur = cur->_next;
		}

		// 头插
		KVNode* tmp = new KVNode(key, value);
		tmp->_next = _tables[index];
		_tables[index] = tmp;

		++_size;

		return true;
	}

	bool Remove(const K& key)
	{
		size_t index = HashFunc(key, _tables.size());
		KVNode* cur = _tables[index];

		if (cur== NULL)
		{
			return false;
		}

		// 1.删除头
		if(cur->_key == key)
		{
			_tables[index] = cur->_next;
			delete cur;
			return true;
		}
		else
		{
			KVNode* prev = cur;
			cur = cur->_next;
			while (cur)
			{
				if (cur->_key == key)
				{
					prev->_next = cur->_next;
					delete cur;
					return true;
				}
				prev = cur;
				cur = cur->_next;
			}
			return false;
		}
	}

	KVNode* Find(const K& key)
	{
		size_t index = HashFunc(key, _tables.size());
		KVNode* cur = _tables[index];
		while (cur)
		{
			if (cur->_key == key)
			{
				return cur;
			}

			cur = cur->_next;
		}

		return NULL;
	}

	size_t HashFunc(const K& key, size_t capacity)
	{
		HashFun hf;
		return hf(key) % capacity;
	}

	void Print()
	{
		for (size_t i = 0; i < _tables.size(); ++i)
		{
			printf("tables[%d]->", i);
			//cout<<"tables["<<i<<
			KVNode* cur = _tables[i];
			while (cur)
			{
				cout<<"["<<cur->_key<<":"<<cur->_value<<"]"<<"->";
				cur = cur->_next;
			}

			cout<<"NULL"<<endl;
		}

		cout<<endl;
	}

protected:
	size_t _GetNextPrime(size_t size)
	{
		static const int _PrimeSize = 28;
		static const unsigned long _PrimeList [_PrimeSize] =
		{
			53ul,         97ul,         193ul,       389ul,       769ul,
			1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
			49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
			1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
			50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
			1610612741ul, 3221225473ul, 4294967291ul
		};

		for (size_t i = 0; i < _PrimeSize; ++i)
		{
			if (_PrimeList[i] > size)
			{
				return _PrimeList[i];
			}
		}

		return _PrimeList[_PrimeSize-1];
	}

	void _CheckCapacity()
	{
		// 载荷因子到1时进行增容，保证效率
		if (_size == _tables.size())
		{
			size_t newCapacity = _GetNextPrime(_size);
			if (_size == newCapacity)
				return;

			vector<KVNode*> newTables;
			newTables.resize(newCapacity);

			for (size_t i = 0; i < _tables.size(); ++i)
			{
				KVNode* cur = _tables[i];
				while (cur)
				{
					// 取节点
					KVNode* tmp = cur;
					cur = cur->_next;

					// 头插
					size_t newIndex =
						HashFunc(tmp->_key, newTables.size());
					tmp->_next = newTables[newIndex];
					newTables[newIndex] = tmp;
				}

				_tables[i] = NULL;
			}

			_tables.swap(newTables);
		}
	}

protected:
	vector<HashTableNode<K,V>*> _tables;
	size_t _size;
};
