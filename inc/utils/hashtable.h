/*
 * Copyright (C) 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UTILS_HASHTABLE_H__
#define __UTILS_HASHTABLE_H__

//typedef struct HashData {
   //void * data;
   //int key;
//} HashData;

//int hashcode (int key, int size)
//{
  //return key % size;
//}

//hashtable_search (
  //HashData ** hash_array,
  //int size,
  //int key)
//{
  //[> get the hash <]
  //int hash_idx = hashcode (key, size);

  ////move in array until an empty
  //while (hash_array[key] != NULL)
    //{

     //if(hashArray[hashIndex]->key == key)
        //return hashArray[hashIndex];

     ////go to next cell
     //++hashIndex;

     ////wrap around the table
     //hashIndex %= SIZE;
  //}

  //return NULL;
//}

//void insert(int key,int data) {

   //struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   //item->data = data;
   //item->key = key;

   ////get the hash
   //int hashIndex = hashCode(key);

   ////move in array until an empty or deleted cell
   //while(hashArray[hashIndex] != NULL && hashArray[hashIndex]->key != -1) {
      ////go to next cell
      //++hashIndex;

      ////wrap around the table
      //hashIndex %= SIZE;
   //}

   //hashArray[hashIndex] = item;
//}

//struct DataItem* delete(struct DataItem* item) {
   //int key = item->key;

   ////get the hash
   //int hashIndex = hashCode(key);

   ////move in array until an empty
   //while(hashArray[hashIndex] != NULL) {

      //if(hashArray[hashIndex]->key == key) {
         //struct DataItem* temp = hashArray[hashIndex];

         ////assign a dummy item at deleted position
         //hashArray[hashIndex] = dummyItem;
         //return temp;
      //}

      ////go to next cell
      //++hashIndex;

      ////wrap around the table
      //hashIndex %= SIZE;
   //}

   //return NULL;
//}

//void display() {
   //int i = 0;

   //for(i = 0; i<SIZE; i++) {

      //if(hashArray[i] != NULL)
         //printf(" (%d,%d)",hashArray[i]->key,hashArray[i]->data);
      //else
         //printf(" ~~ ");
   //}

   //printf("\n");
//}

#endif
