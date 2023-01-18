//
//  SinglyLinkedList.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-10-14.
//

//From: https://github.com/mtrebi/memory-allocators/blob/master/includes/SinglyLinkedList.h

#ifndef SinglyLinkedList_h
#define SinglyLinkedList_h

namespace r2
{
    template <class T>
    class SinglyLinkedList
    {
    public:
        struct Node
        {
            T data;
            Node* next;
        };
        
        Node* head = nullptr;

        static u64 MemorySize(u64 capacity);
    };

    template <class T>
    u64 SinglyLinkedList<T>::MemorySize(u64 capacity)
    {
        return sizeof(SinglyLinkedList<T>) + capacity * sizeof(Node);
    }
    
    namespace sll
    {
        template<typename T> inline void Insert(SinglyLinkedList<T>& list, typename SinglyLinkedList<T>::Node* previousNode, typename SinglyLinkedList<T>::Node* newNode);
        
        template<typename T> inline void Remove(SinglyLinkedList<T>& list, typename SinglyLinkedList<T>::Node* previousNode, typename SinglyLinkedList<T>::Node* deleteNode);
    }
    
    namespace sll
    {
        template<typename T> inline void Insert(SinglyLinkedList<T>& list, typename SinglyLinkedList<T>::Node* previousNode, typename SinglyLinkedList<T>::Node* newNode)
        {
            if (previousNode == nullptr) {
                // Is the first node
                if (list.head != nullptr) {
                    // The list has more elements
                    newNode->next = list.head;
                }else {
                    newNode->next = nullptr;
                }
                list.head = newNode;
            } else {
                if (previousNode->next == nullptr){
                    // Is the last node
                    previousNode->next = newNode;
                    newNode->next = nullptr;
                }else {
                    // Is a middle node
                    newNode->next = previousNode->next;
                    previousNode->next = newNode;
                }
            }
        }
        
        template<typename T> inline void Remove(SinglyLinkedList<T>& list, typename SinglyLinkedList<T>::Node* previousNode, typename SinglyLinkedList<T>::Node* deleteNode)
        {
            if (previousNode == nullptr){
                // Is the first node
                if (deleteNode->next == nullptr){
                    // List only has one element
                    list.head = nullptr;
                }else {
                    // List has more elements
                    list.head = deleteNode->next;
                }
            }else {
                previousNode->next = deleteNode->next;
            }
        }
    }
}

#endif /* SinglyLinkedList_h */
