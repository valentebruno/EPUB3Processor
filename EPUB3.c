#include "EPUB3.h"
#include "EPUB3_private.h"

const char * kEPUB3TypeID = "_EPUB3_t";
const char * kEPUB3MetadataTypeID = "_EPUB3Metadata_t";
const char * kEPUB3ManifestTypeID = "_EPUB3Manifest_t";
const char * kEPUB3ManifestItemTypeID = "_EPUB3ManifestItem_t";
const char * kEPUB3SpineTypeID = "_EPUB3Spine_t";
const char * kEPUB3SpineItemTypeID = "_EPUB3SpineItem_t";


#ifndef PARSE_CONTEXT_STACK_DEPTH
#define PARSE_CONTEXT_STACK_DEPTH 64
#endif

#pragma mark - Public Query API

EXPORT int32_t EPUB3CountOfSequentialResources(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->spine != NULL);
  return epub->spine->linearItemCount;
}

EXPORT EPUB3Error EPUB3GetPathsOfSequentialResources(EPUB3Ref epub, const char ** resources)
{
  assert(epub != NULL);
  assert(epub->spine != NULL);
  
  EPUB3Error error = kEPUB3Success;
  
  if(epub->spine->linearItemCount > 0) {
    int32_t count = 0;
    EPUB3SpineItemListItemPtr itemPtr = epub->spine->head;
    while(itemPtr != NULL) {
      if(itemPtr->item->isLinear) {
        resources[count] = itemPtr->item->manifestItem->href;
        count++;
      }
      itemPtr = itemPtr->next;
    }
    
  }
  
  return error;
}

#pragma mark - Base Object

void EPUB3ObjectRelease(void *object)
{
  if(object == NULL) return;

  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount--;
  if(obj->_type.refCount == 0) {
    free(obj);
    obj = NULL;
  }
}

void EPUB3ObjectRetain(void *object)
{
  if(object == NULL) return;
  
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.refCount++;
}

void * EPUB3ObjectInitWithTypeID(void *object, const char *typeID)
{
  assert(object != NULL);
  EPUB3ObjectRef obj = (EPUB3ObjectRef)object;
  obj->_type.typeID = typeID;
  obj->_type.refCount = 1;
  return obj;
}

#pragma mark - Main EPUB3 Object

EPUB3Ref EPUB3Create()
{
  EPUB3Ref memory = malloc(sizeof(struct EPUB3));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3TypeID);
  memory->metadata = NULL;
  memory->manifest = NULL;
  memory->spine = NULL;
  memory->archive = NULL;
  memory->archivePath = NULL;
  memory->archiveFileCount = 0;
  return memory;
}

EXPORT EPUB3Ref EPUB3CreateWithArchiveAtPath(const char * path)
{
  assert(path != NULL);
  
  EPUB3Ref epub = EPUB3Create();
  unzFile archive = unzOpen(path);
  epub->archive = archive;
  epub->archiveFileCount = EPUB3GetFileCountInArchive(archive);
  epub->archivePath = strdup(path);
  
  return epub;
}

EXPORT EPUB3Error EPUB3InitAndValidate(EPUB3Ref epub)
{
  assert(epub != NULL);
  char * opfPath = NULL;
  EPUB3Error error = EPUB3CopyRootFilePathFromContainer(epub, &opfPath);
  if(error != kEPUB3Success) {
    fprintf(stderr, "Error (%d[%d]) opening and validating epub file at %s.\n", error, __LINE__, epub->archivePath);
  }
  error = EPUB3InitFromOPF(epub, opfPath);
  if(error != kEPUB3Success) {
    fprintf(stderr, "Error (%d[%d]) parsing epub file at %s.\n", error, __LINE__, epub->archivePath);
  }
  return error;
}

EXPORT void EPUB3Retain(EPUB3Ref epub)
{
  if(epub == NULL) return;
  
  EPUB3MetadataRetain(epub->metadata);
  EPUB3ManifestRetain(epub->manifest);
  EPUB3SpineRetain(epub->spine);
  EPUB3ObjectRetain(epub);
}

EXPORT void EPUB3Release(EPUB3Ref epub)
{
  if(epub == NULL) return;
  
  if(epub->_type.refCount == 1) {
    if(epub->archive != NULL) {
      unzClose(epub->archive);
      epub->archive = NULL;
    }
    if(epub->archivePath != NULL) {
      free(epub->archivePath);
      epub->archivePath = NULL;
    }
  }

  EPUB3MetadataRelease(epub->metadata);
  EPUB3ManifestRelease(epub->manifest);
  EPUB3SpineRelease(epub->spine);
  EPUB3ObjectRelease(epub);
}

EXPORT EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub)
{
  assert(epub != NULL);
  
  if(epub->metadata == NULL) {
    return NULL;
  }
  EPUB3MetadataRef copy = EPUB3MetadataCreate();
  (void)EPUB3MetadataSetTitle(copy, epub->metadata->title);
  (void)EPUB3MetadataSetIdentifier(copy, epub->metadata->identifier);
  (void)EPUB3MetadataSetLanguage(copy, epub->metadata->language);
  return copy;
}

void EPUB3SetMetadata(EPUB3Ref epub, EPUB3MetadataRef metadata)
{
  assert(epub != NULL);
  
  if(epub->metadata != NULL) {
    EPUB3MetadataRelease(epub->metadata);
  }
  if(metadata != NULL) {
    EPUB3MetadataRetain(metadata);
  }
  epub->metadata = metadata;
}

void EPUB3SetManifest(EPUB3Ref epub, EPUB3ManifestRef manifest)
{
  assert(epub != NULL);
  
  if(epub->manifest != NULL) {
    EPUB3ManifestRelease(epub->manifest);
  }
  if(manifest != NULL) {
    EPUB3ManifestRetain(manifest);
  }
  epub->manifest = manifest;
}

void EPUB3SetSpine(EPUB3Ref epub, EPUB3SpineRef spine)
{
  assert(epub != NULL);
  
  if(epub->spine != NULL) {
    EPUB3SpineRelease(epub->spine);
  }
  if(spine != NULL) {
    EPUB3SpineRetain(spine);
  }
  epub->spine = spine;
}

void EPUB3SetStringValue(char ** location, const char *value)
{
  if(*location != NULL) {
    free(*location);
  }
  if(value == NULL) {
    *location = NULL;
    return;
  }
  char * valueCopy = strdup(value);
  *location = valueCopy;
}

char * EPUB3CopyStringValue(char ** location)
{
  if(*location == NULL) return NULL;

  char * copy = strdup(*location);
  return copy;
}

EXPORT char * EPUB3CopyTitle(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->metadata != NULL);
  return EPUB3CopyStringValue(&(epub->metadata->title));
}

EXPORT char * EPUB3CopyIdentifier(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->metadata != NULL);
  return EPUB3CopyStringValue(&(epub->metadata->identifier));
}

EXPORT char * EPUB3CopyLanguage(EPUB3Ref epub)
{
  assert(epub != NULL);
  assert(epub->metadata != NULL);
  return EPUB3CopyStringValue(&(epub->metadata->language));
}


#pragma mark - Metadata

EXPORT void EPUB3MetadataRetain(EPUB3MetadataRef metadata)
{
  if(metadata == NULL) return;
  
  EPUB3ObjectRetain(metadata);
}

EXPORT void EPUB3MetadataRelease(EPUB3MetadataRef metadata)
{
  if(metadata == NULL) return;

  if(metadata->_type.refCount == 1) {
    free(metadata->title);
    free(metadata->_uniqueIdentifierID);
    free(metadata->identifier);
    free(metadata->language);
  }
  EPUB3ObjectRelease(metadata);
}

EPUB3MetadataRef EPUB3MetadataCreate()
{
  EPUB3MetadataRef memory = malloc(sizeof(struct EPUB3Metadata));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3MetadataTypeID);
  memory->title = NULL;
  memory->_uniqueIdentifierID = NULL;
  memory->identifier = NULL;
  memory->language = NULL;
  return memory;
}

void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title)
{
  assert(metadata != NULL);
  (void)EPUB3SetStringValue(&(metadata->title), title);
}

void EPUB3MetadataSetIdentifier(EPUB3MetadataRef metadata, const char * identifier)
{
  assert(metadata != NULL);
  (void)EPUB3SetStringValue(&(metadata->identifier), identifier);
}

void EPUB3MetadataSetLanguage(EPUB3MetadataRef metadata, const char * language)
{
  assert(metadata != NULL);
  (void)EPUB3SetStringValue(&(metadata->language), language);
}

#pragma mark - Manifest

EXPORT void EPUB3ManifestRetain(EPUB3ManifestRef manifest)
{
  if(manifest == NULL) return;

  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {
    EPUB3ManifestItemListItemPtr itemPtr = manifest->itemTable[i];
    while(itemPtr != NULL) {
      EPUB3ManifestItemRetain(itemPtr->item);
      itemPtr = itemPtr->next;
    }
  }
  EPUB3ObjectRetain(manifest);
}

EXPORT void EPUB3ManifestRelease(EPUB3ManifestRef manifest)
{
  if(manifest == NULL) return;
  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {

    EPUB3ManifestItemListItemPtr next = manifest->itemTable[i];
    while(next != NULL) {
      EPUB3ManifestItemRelease(next->item);
      EPUB3ManifestItemListItemPtr tmp = next;
      next = tmp->next;
      if(manifest->_type.refCount == 1) {
        free(tmp);
        tmp = NULL;
      }
    }
    if(manifest->_type.refCount == 1) {
      manifest->itemTable[i] = NULL;
    }
  }
  if(manifest->_type.refCount == 1) {
    manifest->itemCount = 0;
  }
  EPUB3ObjectRelease(manifest);
}

EXPORT void EPUB3ManifestItemRetain(EPUB3ManifestItemRef item)
{
  if(item == NULL) return;
  
  EPUB3ObjectRetain(item);
}

EXPORT void EPUB3ManifestItemRelease(EPUB3ManifestItemRef item)
{
  if(item == NULL) return;

  if(item->_type.refCount == 1) {
    free(item->itemId);
    free(item->href);
    free(item->mediaType);
    free(item->properties);
  }
  
  EPUB3ObjectRelease(item);
}

EPUB3ManifestRef EPUB3ManifestCreate()
{
  EPUB3ManifestRef memory = malloc(sizeof(struct EPUB3Manifest));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3ManifestTypeID);
  memory->itemCount = 0;
  for(int i = 0; i < MANIFEST_HASH_SIZE; i++) {
    memory->itemTable[i] = NULL;
  }
  return memory;
}

EPUB3ManifestItemRef EPUB3ManifestItemCreate()
{
  EPUB3ManifestItemRef memory = malloc(sizeof(struct EPUB3ManifestItem));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3ManifestItemTypeID);
  memory->itemId = NULL;
  memory->href = NULL;
  memory->mediaType = NULL;
  memory->properties = NULL;
  return memory;
}

void EPUB3ManifestInsertItem(EPUB3ManifestRef manifest, EPUB3ManifestItemRef item)
{
  assert(manifest != NULL);
  assert(item != NULL);
  assert(item->itemId != NULL);

  EPUB3ManifestItemRetain(item);
  EPUB3ManifestItemListItemPtr itemPtr = EPUB3ManifestFindItemWithId(manifest, item->itemId);
  if(itemPtr == NULL) {
    itemPtr = (EPUB3ManifestItemListItemPtr) malloc(sizeof(struct EPUB3ManifestItemListItem));
    int32_t bucket = SuperFastHash(item->itemId, (int32_t)strlen(item->itemId)) % MANIFEST_HASH_SIZE;
    itemPtr->item = item;
    itemPtr->next = manifest->itemTable[bucket];
    manifest->itemTable[bucket] = itemPtr;
    manifest->itemCount++;
  } else {
    EPUB3ManifestItemRelease(itemPtr->item);
    itemPtr->item = item;
  }
}

EPUB3ManifestItemRef EPUB3ManifestCopyItemWithId(EPUB3ManifestRef manifest, const char * itemId)
{
  assert(manifest != NULL);
  assert(itemId != NULL);

  EPUB3ManifestItemListItemPtr itemPtr = EPUB3ManifestFindItemWithId(manifest, itemId);
  
  if(itemPtr == NULL) {
    return NULL;
  }
  
  EPUB3ManifestItemRef item = itemPtr->item;
  EPUB3ManifestItemRef copy = EPUB3ManifestItemCreate();
  copy->itemId = item->itemId != NULL ? strdup(item->itemId) : NULL;
  copy->href = item->href != NULL ? strdup(item->href) : NULL;
  copy->mediaType = item->mediaType != NULL ? strdup(item->mediaType) : NULL;
  copy->properties = item->properties != NULL ? strdup(item->properties) : NULL;
  return copy;
}

EPUB3ManifestItemListItemPtr EPUB3ManifestFindItemWithId(EPUB3ManifestRef manifest, const char * itemId)
{
  assert(manifest != NULL);
  assert(itemId != NULL);

  int32_t bucket = SuperFastHash(itemId, (int32_t)strlen(itemId)) % MANIFEST_HASH_SIZE;
  EPUB3ManifestItemListItemPtr itemPtr = manifest->itemTable[bucket];
  while(itemPtr != NULL) {
    if(strcmp(itemId, itemPtr->item->itemId) == 0) {
      return itemPtr;
    }
    itemPtr = itemPtr->next;
  }
  return NULL;
}

#pragma mark - Spine

EPUB3SpineRef EPUB3SpineCreate()
{
  EPUB3SpineRef memory = malloc(sizeof(struct EPUB3Spine));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3SpineTypeID);
  memory->itemCount = 0;
  memory->linearItemCount = 0;
  memory->head = NULL;
  memory->tail = NULL;
  return memory;
}

EXPORT void EPUB3SpineRetain(EPUB3SpineRef spine)
{
  if(spine == NULL) return;

  EPUB3SpineItemListItemPtr itemPtr;
  for(itemPtr = spine->head; itemPtr != NULL; itemPtr = itemPtr->next) {
    EPUB3SpineItemRetain(itemPtr->item);
  }
  EPUB3ObjectRetain(spine);
}

EXPORT void EPUB3SpineRelease(EPUB3SpineRef spine)
{
  if(spine == NULL) return;
  if(spine->_type.refCount == 1) {
    EPUB3SpineItemListItemPtr itemPtr = spine->head;
    int totalItemsToFree = spine->itemCount;
    while(itemPtr != NULL) {
      assert(--totalItemsToFree >= 0);
      EPUB3SpineItemRelease(itemPtr->item);
      EPUB3SpineItemListItemPtr tmp = itemPtr;
      itemPtr = itemPtr->next;
      spine->head = itemPtr;
      free(tmp);
      tmp = NULL;
    }
    spine->itemCount = 0;
    spine->linearItemCount = 0;
  }
  EPUB3ObjectRelease(spine);
}

EPUB3SpineItemRef EPUB3SpineItemCreate()
{
  EPUB3SpineItemRef memory = malloc(sizeof(struct EPUB3SpineItem));
  memory = EPUB3ObjectInitWithTypeID(memory, kEPUB3SpineItemTypeID);
  memory->isLinear = kEPUB3_NO;
  memory->idref = NULL;
  memory->manifestItem = NULL;
  return memory;
}

EXPORT void EPUB3SpineItemRetain(EPUB3SpineItemRef item)
{
  if(item == NULL) return;
  EPUB3ObjectRetain(item);
}

EXPORT void EPUB3SpineItemRelease(EPUB3SpineItemRef item)
{
  if(item == NULL) return;
  
  if(item->_type.refCount == 1) {
    item->manifestItem = NULL; // zero weak ref
    if(item->idref != NULL) {
      free(item->idref);
    }
  }
  
  EPUB3ObjectRelease(item);
}

void EPUB3SpineItemSetManifestItem(EPUB3SpineItemRef spineItem, EPUB3ManifestItemRef manifestItem)
{
  assert(spineItem != NULL);
  spineItem->manifestItem = manifestItem;
  spineItem->idref = strdup(manifestItem->itemId);
}

void EPUB3SpineAppendItem(EPUB3SpineRef spine, EPUB3SpineItemRef item)
{
  assert(spine != NULL);
  assert(item != NULL);
  
  EPUB3SpineItemRetain(item);
  EPUB3SpineItemListItemPtr itemPtr = (EPUB3SpineItemListItemPtr) calloc(1, sizeof(struct EPUB3SpineItemListItem));
  itemPtr->item = item;
  
  if(spine->head == NULL) {
    // First item
    spine->head = itemPtr;
    spine->tail = itemPtr;
  } else {
    spine->tail->next = itemPtr;
    spine->tail = itemPtr;
  }
  spine->itemCount++;
}

#pragma mark - XML Parsing

EPUB3Error EPUB3InitFromOPF(EPUB3Ref epub, const char * opfFilename)
{
  assert(epub != NULL);
  assert(opfFilename != NULL);
  
  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;
  
  if(epub->metadata == NULL) {
    epub->metadata = EPUB3MetadataCreate();
  }
  
  if(epub->manifest == NULL) {
    epub->manifest = EPUB3ManifestCreate();
  }
  
  if(epub->spine == NULL) {
    epub->spine = EPUB3SpineCreate();
  }
  
  void *buffer = NULL;
  uint32_t bufferSize = 0;
  uint32_t bytesCopied;
  
  EPUB3Error error = kEPUB3Success;
  
  error = EPUB3CopyFileIntoBuffer(epub, &buffer, &bufferSize, &bytesCopied, opfFilename);
  if(error == kEPUB3Success) {
    error = EPUB3ParseFromOPFData(epub, buffer, bufferSize);
    free(buffer);
  }
  return error;
}

void EPUB3SaveParseContext(EPUB3OPFParseContextPtr *ctxPtr, EPUB3OPFParseState state, const xmlChar * tagName, int32_t attrCount, char ** attrs, EPUB3Bool shouldParseTextNode)
{
  (*ctxPtr)++;
  (*ctxPtr)->state = state;
  (*ctxPtr)->tagName = tagName;
  (*ctxPtr)->attributeCount = attrCount;
  (*ctxPtr)->attributes = attrs;
  (*ctxPtr)->shouldParseTextNode = shouldParseTextNode;
}

void EPUB3PopAndFreeParseContext(EPUB3OPFParseContextPtr *contextPtr)
{
  EPUB3OPFParseContextPtr ctx = (*contextPtr);
  (*contextPtr)--;
  for (int i = 0; i < ctx->attributeCount; i++) {
    char * key = ctx->attributes[i * 2];
    char * val = ctx->attributes[i * 2 + 1];
    free(key);
    free(val);
  }
}

EPUB3Error EPUB3ProcessXMLReaderNodeForMetadataInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *context)
{
  assert(epub != NULL);
  assert(reader != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes nodeType = xmlTextReaderNodeType(reader);

  switch(nodeType)
  {
    case XML_READER_TYPE_ELEMENT:
    {
      if(!xmlTextReaderIsEmptyElement(reader)) {
        (void)EPUB3SaveParseContext(context, kEPUB3OPFStateMetadata, name, 0, NULL, kEPUB3_YES);

        // Only parse text node for the identifier marked as unique-identifier in the package tag
        // see: http://idpf.org/epub/30/spec/epub30-publications.html#sec-opf-dcidentifier
        if(xmlStrcmp(name, BAD_CAST "identifier") == 0) {
          if(xmlTextReaderHasAttributes(reader)) {
            xmlChar * itemId = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
            if(itemId == NULL) {
              (*context)->shouldParseTextNode = kEPUB3_NO;
            }
            else if(itemId != NULL && xmlStrcmp(itemId, BAD_CAST epub->metadata->_uniqueIdentifierID) != 0) {
              (*context)->shouldParseTextNode = kEPUB3_NO; 
              free(itemId);
            }
          }
        }

      }
      break;
    }
    case XML_READER_TYPE_TEXT:
    {
      const xmlChar *value = xmlTextReaderValue(reader);
      if(value != NULL && (*context)->shouldParseTextNode) {
        if(xmlStrcmp((*context)->tagName, BAD_CAST "title") == 0) {
          (void)EPUB3MetadataSetTitle(epub->metadata, (const char *)value);
        }
        else if(xmlStrcmp((*context)->tagName, BAD_CAST "identifier") == 0) {
          (void)EPUB3MetadataSetIdentifier(epub->metadata, (const char *)value);
        }
        else if(xmlStrcmp((*context)->tagName, BAD_CAST "language") == 0) {
          (void)EPUB3MetadataSetLanguage(epub->metadata, (const char *)value);
        }
      }
      break;
    }
    case XML_READER_TYPE_END_ELEMENT:
    {
      (void)EPUB3PopAndFreeParseContext(context);
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error EPUB3ProcessXMLReaderNodeForManifestInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *context)
{
  assert(epub != NULL);
  assert(reader != NULL);
  
  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes nodeType = xmlTextReaderNodeType(reader);
  
  switch(nodeType)
  {
    case XML_READER_TYPE_ELEMENT:
    {
      if(!xmlTextReaderIsEmptyElement(reader)) {
        (void)EPUB3SaveParseContext(context, kEPUB3OPFStateManifest, name, 0, NULL, kEPUB3_YES);
      } else {
        if(xmlStrcmp(name, BAD_CAST "item") == 0) {
          EPUB3ManifestItemRef newItem = EPUB3ManifestItemCreate();
          newItem->itemId = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "id");
          newItem->href = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "href");
          newItem->mediaType = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "media-type");
          EPUB3ManifestInsertItem(epub->manifest, newItem);
        }
      }
      break;
    }
    case XML_READER_TYPE_TEXT:
    {
      break;
    }
    case XML_READER_TYPE_END_ELEMENT:
    {
      (void)EPUB3PopAndFreeParseContext(context);
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error EPUB3ProcessXMLReaderNodeForSpineInOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *context)
{
  assert(epub != NULL);
  assert(reader != NULL);
  
  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes nodeType = xmlTextReaderNodeType(reader);
  
  switch(nodeType)
  {
    case XML_READER_TYPE_ELEMENT:
    {
      if(!xmlTextReaderIsEmptyElement(reader)) {
        (void)EPUB3SaveParseContext(context, kEPUB3OPFStateManifest, name, 0, NULL, kEPUB3_YES);
      } else {
        if(xmlStrcmp(name, BAD_CAST "itemref") == 0) {
          EPUB3SpineItemRef newItem = EPUB3SpineItemCreate();
          xmlChar * linear = xmlTextReaderGetAttribute(reader, BAD_CAST "linear");
          
          if(linear == NULL || xmlStrcmp(linear, BAD_CAST "yes") == 0) {
            newItem->isLinear = kEPUB3_YES;
            epub->spine->linearItemCount++;
          }
          free(linear);
          newItem->idref = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "idref");
          if(newItem->idref != NULL) {
            EPUB3ManifestItemListItemPtr manifestPtr = EPUB3ManifestFindItemWithId(epub->manifest, newItem->idref);
            if(manifestPtr == NULL) {
              newItem->manifestItem = NULL;
            } else {
              newItem->manifestItem = manifestPtr->item;
            }
          }
          EPUB3SpineAppendItem(epub->spine, newItem);
        }
      }
      break;
    }
    case XML_READER_TYPE_TEXT:
    {
      break;
    }
    case XML_READER_TYPE_END_ELEMENT:
    {
      (void)EPUB3PopAndFreeParseContext(context);
      break;
    }
    default: break;
  }
  return error;
}

EPUB3Error EPUB3ParseXMLReaderNodeForOPF(EPUB3Ref epub, xmlTextReaderPtr reader, EPUB3OPFParseContextPtr *currentContext)
{
  assert(epub != NULL);
  assert(reader != NULL);
  assert(*currentContext != NULL);

  EPUB3Error error = kEPUB3Success;
  const xmlChar *name = xmlTextReaderConstLocalName(reader);
  xmlReaderTypes currentNodeType = xmlTextReaderNodeType(reader);
  
  if(name != NULL && currentNodeType != XML_READER_TYPE_COMMENT) {
    switch((*currentContext)->state)
    {
      case kEPUB3OPFStateRoot:
      {
//        fprintf(stdout, "ROOT: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_ELEMENT) {
          if(xmlStrcmp(name, BAD_CAST "package") == 0 && xmlTextReaderHasAttributes(reader)) {
            if(epub->metadata->_uniqueIdentifierID != NULL) {
              free(epub->metadata->_uniqueIdentifierID);
            }
            epub->metadata->_uniqueIdentifierID = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "unique-identifier");
          }
          else if(xmlStrcmp(name, BAD_CAST "metadata") == 0) {
            (void)EPUB3SaveParseContext(currentContext, kEPUB3OPFStateMetadata, name, 0, NULL, kEPUB3_YES);
          }
          else if(xmlStrcmp(name, BAD_CAST "manifest") == 0) {
            (void)EPUB3SaveParseContext(currentContext, kEPUB3OPFStateManifest, name, 0, NULL, kEPUB3_YES);
          }
          else if(xmlStrcmp(name, BAD_CAST "spine") == 0) {
            (void)EPUB3SaveParseContext(currentContext, kEPUB3OPFStateSpine, name, 0, NULL, kEPUB3_YES);
          }
        }
        break;
      }
      case kEPUB3OPFStateMetadata:
      {
//        fprintf(stdout, "METADATA: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "metadata") == 0) {
          (void)EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = EPUB3ProcessXMLReaderNodeForMetadataInOPF(epub, reader, currentContext);
        }
        break;
      }
      case kEPUB3OPFStateManifest:
      {
//        fprintf(stdout, "MANIFEST: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "manifest") == 0) {
          (void)EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = EPUB3ProcessXMLReaderNodeForManifestInOPF(epub, reader, currentContext);
        }
        break;
      }
      case kEPUB3OPFStateSpine:
      {
//        fprintf(stdout, "SPINE: %s\n", name);
        if(currentNodeType == XML_READER_TYPE_END_ELEMENT && xmlStrcmp(name, BAD_CAST "spine") == 0) {
          (void)EPUB3PopAndFreeParseContext(currentContext);
        } else {
          error = EPUB3ProcessXMLReaderNodeForSpineInOPF(epub, reader, currentContext);
        }
        break;
      }
      default: break;
    }
  }
  return error;
}

EPUB3Error EPUB3ParseFromOPFData(EPUB3Ref epub, void * buffer, uint32_t bufferSize)
{
  assert(epub != NULL);
  assert(buffer != NULL);
  assert(bufferSize > 0);

  EPUB3Error error = kEPUB3Success;
  xmlInitParser();
  xmlTextReaderPtr reader = NULL;
  reader = xmlReaderForMemory(buffer, bufferSize, NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NONET);
  // (void)xmlTextReaderSetParserProp(reader, XML_PARSER_VALIDATE, 1);
  if(reader != NULL) {
    EPUB3OPFParseContext contextStack[PARSE_CONTEXT_STACK_DEPTH];
    EPUB3OPFParseContextPtr currentContext = &contextStack[0];

    int retVal = xmlTextReaderRead(reader);
    currentContext->state = kEPUB3OPFStateRoot;
    currentContext->tagName = xmlTextReaderConstName(reader);
    while(retVal == 1)
    {
      error = EPUB3ParseXMLReaderNodeForOPF(epub, reader, &currentContext);
      retVal = xmlTextReaderRead(reader);
    }
    if(retVal < 0) {
      error = kEPUB3XMLParseError;
    }
  } else {
    error = kEPUB3XMLReadFromBufferError;
  }
  xmlFreeTextReader(reader);
  xmlCleanupParser();
  return error;
}

#pragma mark - Validation

EPUB3Error EPUB3ValidateMimetype(EPUB3Ref epub)
{
  assert(epub != NULL);
  
  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  EPUB3Error status = kEPUB3InvalidMimetypeError;
  static const char * requiredMimetype = "application/epub+zip";
  static const int stringLength = 20;
  char buffer[stringLength];
  
  if(unzGoToFirstFile(epub->archive) == UNZ_OK) {
    uint32_t stringLength = (uint32_t)strlen(requiredMimetype);
    if(unzOpenCurrentFile(epub->archive) == UNZ_OK) {
      int byteCount = unzReadCurrentFile(epub->archive, buffer, stringLength);
      if(byteCount == stringLength) {
        if(strncmp(requiredMimetype, buffer, stringLength) == 0) {
          status = kEPUB3Success;
        }
      }
    }
    unzCloseCurrentFile(epub->archive);
  }
  return status;
}

EPUB3Error EPUB3CopyRootFilePathFromContainer(EPUB3Ref epub, char ** rootPath)
{
  assert(epub != NULL);
  
  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;
  
  static const char *containerFilename = "META-INF/container.xml";

  void *buffer = NULL;
  uint32_t bufferSize = 0;
  uint32_t bytesCopied;

  xmlTextReaderPtr reader = NULL;
  EPUB3Bool foundPath = kEPUB3_NO;
  
  EPUB3Error error = kEPUB3Success;

  error = EPUB3CopyFileIntoBuffer(epub, &buffer, &bufferSize, &bytesCopied, containerFilename);
  if(error == kEPUB3Success) {
    reader = xmlReaderForMemory(buffer, bufferSize, "", NULL, XML_PARSE_RECOVER);
    if(reader != NULL) {
      int retVal;
      while((retVal = xmlTextReaderRead(reader)) == 1)
      {
        const char *rootFileName = "rootfile";
        const xmlChar *name = xmlTextReaderConstLocalName(reader);

        if(xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlStrcmp(name, BAD_CAST rootFileName) == 0) {
          xmlChar *fullPath = xmlTextReaderGetAttribute(reader, BAD_CAST "full-path");
          if(fullPath != NULL) {
            // TODD: validate that the full-path attribute is of the form path-rootless
            //       see http://idpf.org/epub/30/spec/epub30-ocf.html#sec-container-metainf-container.xml
            foundPath = kEPUB3_YES;
            *rootPath = (char *)fullPath;
          } else {
            // The spec requires the full-path attribute
            error = kEPUB3XMLXDocumentInvalidError;
          }
          break;
        }
      }
      if(retVal < 0) {
        error = kEPUB3XMLParseError;
      }
      if(!foundPath) {
        error = kEPUB3XMLXElementNotFoundError;
      }
    } else {
      error = kEPUB3XMLReadFromBufferError;
    }
    free(buffer);
  }
  xmlFreeTextReader(reader);
  return error;
}

EPUB3Error EPUB3ValidateFileExistsAndSeekInArchive(EPUB3Ref epub, const char * filename)
{
  assert(epub != NULL);
  assert(filename != NULL);
  
  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;

  EPUB3Error error = kEPUB3FileNotFoundInArchiveError;
  if(unzLocateFile(epub->archive, filename, 1) == UNZ_OK) {
    error = kEPUB3Success;
  }
  return error;
}

#pragma mark - Utility functions

EPUB3Error EPUB3CopyFileIntoBuffer(EPUB3Ref epub, void **buffer, uint32_t *bufferSize, uint32_t *bytesCopied, const char * filename)
{
  assert(epub != NULL);
  assert(filename != NULL);
  assert(buffer != NULL);
  
  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;
  
  EPUB3Error error = kEPUB3InvalidArgumentError;
  if(filename != NULL) {
    uint32_t bufSize = 1;
    error = EPUB3GetUncompressedSizeOfFileInArchive(epub, &bufSize, filename);
    if(error == kEPUB3Success) {
      if(unzOpenCurrentFile(epub->archive) == UNZ_OK) {
        *buffer = calloc(bufSize, sizeof(char));
        int32_t copied = unzReadCurrentFile(epub->archive, *buffer, bufSize);
        if(copied >= 0) {
          if(bytesCopied != NULL) {
            *bytesCopied = copied;
          }
          if(bufferSize != NULL) {
            *bufferSize = bufSize;
          }
          error = kEPUB3Success;
        } else {
          free(*buffer);
          error = kEPUB3FileReadFromArchiveError;
        }
      }
    }
  }
  return error;
}

EPUB3Error EPUB3GetUncompressedSizeOfFileInArchive(EPUB3Ref epub, uint32_t *uncompressedSize, const char *filename)
{
  assert(epub != NULL);
  assert(filename != NULL);
  assert(uncompressedSize != NULL);
  
  if(epub->archive == NULL) return kEPUB3ArchiveUnavailableError;
  
  EPUB3Error error = EPUB3ValidateFileExistsAndSeekInArchive(epub, filename);
  if(error == kEPUB3Success) {
    unz_file_info fileInfo;
    if(unzGetCurrentFileInfo(epub->archive, &fileInfo, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK) {
      *uncompressedSize = (uint32_t)fileInfo.uncompressed_size;
      error = kEPUB3Success;
    }
  }
  return error;
}

uint32_t EPUB3GetFileCountInArchive(EPUB3Ref epub)
{
  unz_global_info gi;
	int err = unzGetGlobalInfo(epub->archive, &gi);
	if (err != UNZ_OK)
    return err;
	
	return (uint32_t)gi.number_entry;
}
