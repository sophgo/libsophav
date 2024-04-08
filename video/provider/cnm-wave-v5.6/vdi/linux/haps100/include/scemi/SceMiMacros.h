/******************************************************************************
 Copyright © 2009 Synopsys, Inc. This Example and the associated documentation
 are confidential and proprietary to Synopsys, Inc. Your use or disclosure of
 this example is subject to the terms and conditions of a written license
 agreement between you, or your company, and Synopsys, Inc.
 *******************************************************************************
 Title      : Commonly used macros
 Project    : SCE-MI 2 Implementation
 ******************************************************************************/
/**
 * @file   SceMiMacros.h
 * @author  Roshan Antony Tauro
 * @date   2006/10/17
 *
 * @brief  This File contains definitions of various macros used in the API.
 *
 *
 */
/******************************************************************************
 $Id: //chipit/chipit/main/release/tools/scemi2/sw/include/SceMiMacros.h#2 $ $Author: mlange $ $DateTime: 2014/11/14 01:29:14 $

 $Log: SceMiMacros.h,v $
 Revision 1.9  2009/02/20 13:20:03  rtauro
 Added copyright info.

 Revision 1.8  2009/01/20 14:06:52  mmueller
 move macro PTHREADTESTCANCEL isn't a global one

 Revision 1.7  2009/01/20 13:56:23  mmueller
 change PTHREADTESTCANCEL

 Revision 1.6  2009/01/20 13:54:09  rtauro
 Fixed errors related to threads.

 Revision 1.5  2009/01/20 13:35:26  rtauro
 Fixed errors related to threads.

 Revision 1.4  2008/12/12 16:15:22  rtauro
 Improved Error Handling.

 Revision 1.3  2007/11/05 15:08:08  rtauro
 First Revision



 ******************************************************************************/
#ifndef __SceMiMacros_h__
#define __SceMiMacros_h__

/* Declaraion of the List Structure */
#define DECLARE_LIST(NAME, TYPE)	\
typedef struct S_##NAME				\
{									\
	size_t size;					\
	size_t count;					\
	TYPE *data;						\
	int   destroy;                  \
} NAME;

/* Declaration of the List Functions */
#define DECLARE_LIST_FUNCTIONS(NAME, TYPE)	\
void NAME##_create(NAME *, size_t);	\
void NAME##_destroy(NAME *);			\
void NAME##_append(NAME *, TYPE *);	\
int NAME##_remove(NAME *, size_t);	\
TYPE *NAME##_get(NAME *, size_t);	\
size_t NAME##_size(NAME *);			\
int NAME##_find(NAME *, TYPE *);

/* Initialization of the List */
#define INITIALIZE_LIST(NAME, VARIABLE)	\
VARIABLE = (NAME *)malloc(sizeof(NAME))

/* Defination of the List Functions */
#define IMPLEMENT_LIST(NAME, TYPE)									\
void NAME##_create(NAME *pointer, size_t init_size)					\
{																	\
	pointer->data = (TYPE *)malloc(sizeof(TYPE) * init_size);		\
	pointer->size = init_size;										\
	pointer->count = 0;												\
}																	\
																	\
void NAME##_destroy(NAME *pointer)									\
{																	\
	if(pointer)														\
	{																\
		free(pointer->data);										\
		pointer->size = 0;											\
		pointer->count = 0;											\
	}																\
}																	\
																	\
void NAME##_append(NAME *pointer, TYPE *data)						\
{																	\
	if (pointer->data)												\
	{																\
		if(pointer->count == pointer->size)							\
		{															\
			pointer->size = 2 * pointer->size;						\
			pointer->data = (TYPE *)realloc(pointer->data, sizeof(TYPE) * (pointer->size));	\
		}															\
		memcpy(pointer->data + pointer->count, data, sizeof(TYPE));	\
		++ pointer->count;											\
  }																	\
}																	\
																	\
int NAME##_remove(NAME *pointer, size_t pos)						\
{																	\
	if(pointer)														\
	{																\
		if(pos >= pointer->count)									\
			return -1;												\
		memmove(pointer->data + pos, pointer->data + pos + 1, sizeof(TYPE) * (pointer->size - pos - 1));	\
		-- pointer->count;											\
		return 0;													\
	}																\
	return -1;														\
}																	\
																	\
TYPE *NAME##_get(NAME *pointer, size_t pos)							\
{																	\
	if(pointer)														\
	{																\
		if(pos >= pointer->count)									\
			return NULL;											\
		return pointer->data + pos;									\
	}																\
	return NULL;													\
}																	\
																	\
size_t NAME##_size(NAME *pointer)									\
{																	\
	return pointer->count;											\
}																	\
																	\
int NAME##_find(NAME *pointer, TYPE *element)						\
{																	\
	size_t i;														\
	for(i = 0; i < pointer->count; i ++)							\
		if(memcmp(element, pointer->data + i, sizeof(TYPE)) == 0) return (int)i;	\
	return -1;														\
}

/* Error Handling Macro */
#define HANDLE_ERROR(ERROR_OBJECT, CULPRIT, MESSAGE, TYPE, RETURN_VALUE)	\
if(ERROR_OBJECT != NULL)															\
{																			\
	ERROR_OBJECT = setSceMiErrorObject(ERROR_OBJECT, CULPRIT, MESSAGE, TYPE);	\
	return RETURN_VALUE;													\
}																			\
else																		\
{																			\
	ERROR_OBJECT = setSceMiErrorObject(ERROR_OBJECT, CULPRIT, MESSAGE, TYPE);	\
	if(sceMiErrorHandler) {                                         \
            sceMiErrorHandler(sceMiErrorContext, ERROR_OBJECT);         \
            return RETURN_VALUE;                                        \
        }                                                               \
	else {                                                          \
            fprintf (stderr, "SCE-MI Error[ %d ]: Function: %s | %s\n", (int) ERROR_OBJECT->Type, ERROR_OBJECT->Culprit, ERROR_OBJECT->Message);\
            exit(EXIT_FAILURE);											\
        }                                       \
}

/* Info Handling Macro */
#define HANDLE_INFO(INFO_OBJECT, ORIGINATOR, MESSAGE, TYPE)					\
if(INFO_OBJECT)																\
{																			\
	INFO_OBJECT = setSceMiInfoObject(INFO_OBJECT, ORIGINATOR, MESSAGE, TYPE);	\
}																			\
else																		\
{																			\
	INFO_OBJECT = setSceMiInfoObject(INFO_OBJECT, ORIGINATOR, MESSAGE, TYPE);	\
	if(sceMiInfoHandler)													\
		sceMiInfoHandler(sceMiInfoContext, INFO_OBJECT);								\
	else																	\
		fprintf (stderr, "SCE-MI Info[ %d ]: Function: %s | %s\n", (int) INFO_OBJECT->Type, INFO_OBJECT->Originator, INFO_OBJECT->Message);\
}

#ifndef tipihc_indent
#define tipihc_indent() uiGlobalSceMiDebugIndentLevel+=3
#endif
#ifndef tipihc_unindent
#define tipihc_unindent() uiGlobalSceMiDebugIndentLevel-=3
#endif

#endif

