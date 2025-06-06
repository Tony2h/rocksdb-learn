// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
package org.rocksdb;

import java.util.Arrays;
import java.util.Map;
import java.util.Objects;

/**
 * TableProperties contains read-only properties of its associated
 * table.
 */
public class TableProperties {
  private final long dataSize;
  private final long indexSize;
  private final long indexPartitions;
  private final long topLevelIndexSize;
  private final long indexKeyIsUserKey;
  private final long indexValueIsDeltaEncoded;
  private final long filterSize;
  private final long rawKeySize;
  private final long rawValueSize;
  private final long numDataBlocks;
  private final long numEntries;
  private final long numDeletions;
  private final long numMergeOperands;
  private final long numRangeDeletions;
  private final long formatVersion;
  private final long fixedKeyLen;
  private final long columnFamilyId;
  private final long creationTime;
  private final long oldestKeyTime;
  private final long slowCompressionEstimatedDataSize;
  private final long fastCompressionEstimatedDataSize;
  private final long externalSstFileGlobalSeqnoOffset;
  private final byte[] columnFamilyName;
  private final String filterPolicyName;
  private final String comparatorName;
  private final String mergeOperatorName;
  private final String prefixExtractorName;
  private final String propertyCollectorsNames;
  private final String compressionName;
  private final Map<String, String> userCollectedProperties;
  private final Map<String, String> readableProperties;

  /**
   * Access is package private as this will only be constructed from
   * C++ via JNI and for testing.
   */
  @SuppressWarnings("PMD.ArrayIsStoredDirectly")
  TableProperties(final long dataSize, final long indexSize, final long indexPartitions,
      final long topLevelIndexSize, final long indexKeyIsUserKey,
      final long indexValueIsDeltaEncoded, final long filterSize, final long rawKeySize,
      final long rawValueSize, final long numDataBlocks, final long numEntries,
      final long numDeletions, final long numMergeOperands, final long numRangeDeletions,
      final long formatVersion, final long fixedKeyLen, final long columnFamilyId,
      final long creationTime, final long oldestKeyTime,
      final long slowCompressionEstimatedDataSize, final long fastCompressionEstimatedDataSize,
      final long externalSstFileGlobalSeqnoOffset, final byte[] columnFamilyName,
      final String filterPolicyName, final String comparatorName, final String mergeOperatorName,
      final String prefixExtractorName, final String propertyCollectorsNames,
      final String compressionName, final Map<String, String> userCollectedProperties,
      final Map<String, String> readableProperties) {
    this.dataSize = dataSize;
    this.indexSize = indexSize;
    this.indexPartitions = indexPartitions;
    this.topLevelIndexSize = topLevelIndexSize;
    this.indexKeyIsUserKey = indexKeyIsUserKey;
    this.indexValueIsDeltaEncoded = indexValueIsDeltaEncoded;
    this.filterSize = filterSize;
    this.rawKeySize = rawKeySize;
    this.rawValueSize = rawValueSize;
    this.numDataBlocks = numDataBlocks;
    this.numEntries = numEntries;
    this.numDeletions = numDeletions;
    this.numMergeOperands = numMergeOperands;
    this.numRangeDeletions = numRangeDeletions;
    this.formatVersion = formatVersion;
    this.fixedKeyLen = fixedKeyLen;
    this.columnFamilyId = columnFamilyId;
    this.creationTime = creationTime;
    this.oldestKeyTime = oldestKeyTime;
    this.slowCompressionEstimatedDataSize = slowCompressionEstimatedDataSize;
    this.fastCompressionEstimatedDataSize = fastCompressionEstimatedDataSize;
    this.externalSstFileGlobalSeqnoOffset = externalSstFileGlobalSeqnoOffset;
    this.columnFamilyName = columnFamilyName;
    this.filterPolicyName = filterPolicyName;
    this.comparatorName = comparatorName;
    this.mergeOperatorName = mergeOperatorName;
    this.prefixExtractorName = prefixExtractorName;
    this.propertyCollectorsNames = propertyCollectorsNames;
    this.compressionName = compressionName;
    this.userCollectedProperties = userCollectedProperties;
    this.readableProperties = readableProperties;
  }

  /**
   * Get the total size of all data blocks.
   *
   * @return the total size of all data blocks.
   */
  public long getDataSize() {
    return dataSize;
  }

  /**
   * Get the size of index block.
   *
   * @return the size of index block.
   */
  public long getIndexSize() {
    return indexSize;
  }

  /**
   * Get the total number of index partitions
   * if {@link IndexType#kTwoLevelIndexSearch} is used.
   *
   * @return the total number of index partitions.
   */
  @SuppressWarnings("PMD.MethodReturnsInternalArray")
  public long getIndexPartitions() {
    return indexPartitions;
  }

  /**
   * Size of the top-level index
   * if {@link IndexType#kTwoLevelIndexSearch} is used.
   *
   * @return the size of the top-level index.
   */
  public long getTopLevelIndexSize() {
    return topLevelIndexSize;
  }

  /**
   * Whether the index key is user key.
   * Otherwise it includes 8 byte of sequence
   * number added by internal key format.
   *
   * @return the index key
   */
  public long getIndexKeyIsUserKey() {
    return indexKeyIsUserKey;
  }

  /**
   * Whether delta encoding is used to encode the index values.
   *
   * @return whether delta encoding is used to encode the index values.
   */
  public long getIndexValueIsDeltaEncoded() {
    return indexValueIsDeltaEncoded;
  }

  /**
   * Get the size of filter block.
   *
   * @return the size of filter block.
   */
  public long getFilterSize() {
    return filterSize;
  }

  /**
   * Get the total raw key size.
   *
   * @return the total raw key size.
   */
  public long getRawKeySize() {
    return rawKeySize;
  }

  /**
   * Get the total raw value size.
   *
   * @return the total raw value size.
   */
  public long getRawValueSize() {
    return rawValueSize;
  }

  /**
   * Get the number of blocks in this table.
   *
   * @return the number of blocks in this table.
   */
  public long getNumDataBlocks() {
    return numDataBlocks;
  }

  /**
   * Get the number of entries in this table.
   *
   * @return the number of entries in this table.
   */
  public long getNumEntries() {
    return numEntries;
  }

  /**
   * Get the number of deletions in the table.
   *
   * @return the number of deletions in the table.
   */
  public long getNumDeletions() {
    return numDeletions;
  }

  /**
   * Get the number of merge operands in the table.
   *
   * @return the number of merge operands in the table.
   */
  public long getNumMergeOperands() {
    return numMergeOperands;
  }

  /**
   * Get the number of range deletions in this table.
   *
   * @return the number of range deletions in this table.
   */
  public long getNumRangeDeletions() {
    return numRangeDeletions;
  }

  /**
   * Get the format version, reserved for backward compatibility.
   *
   * @return the format version.
   */
  public long getFormatVersion() {
    return formatVersion;
  }

  /**
   * Get the length of the keys.
   *
   * @return 0 when the key is variable length, otherwise number of
   *     bytes for each key.
   */
  public long getFixedKeyLen() {
    return fixedKeyLen;
  }

  /**
   * Get the ID of column family for this SST file,
   * corresponding to the column family identified by
   * {@link #getColumnFamilyName()}.
   *
   * @return the id of the column family.
   */
  public long getColumnFamilyId() {
    return columnFamilyId;
  }

  /**
   * The time when the SST file was created.
   * Since SST files are immutable, this is equivalent
   * to last modified time.
   *
   * @return the created time.
   */
  public long getCreationTime() {
    return creationTime;
  }

  /**
   * Get the timestamp of the earliest key.
   *
   * @return 0 means unknown, otherwise the timestamp.
   */
  public long getOldestKeyTime() {
    return oldestKeyTime;
  }

  /**
   * Get the estimated size of data blocks compressed with a relatively slower
   * compression algorithm.
   *
   * @return 0 means unknown, otherwise the timestamp.
   */
  public long getSlowCompressionEstimatedDataSize() {
    return slowCompressionEstimatedDataSize;
  }

  /**
   * Get the estimated size of data blocks compressed with a relatively faster
   * compression algorithm.
   *
   * @return 0 means unknown, otherwise the timestamp.
   */
  public long getFastCompressionEstimatedDataSize() {
    return fastCompressionEstimatedDataSize;
  }

  /**
   * Get the name of the column family with which this
   * SST file is associated.
   *
   * @return the name of the column family, or null if the
   *     column family is unknown.
   */
  @SuppressWarnings("PMD.MethodReturnsInternalArray")
  /*@Nullable*/ public byte[] getColumnFamilyName() {
    return columnFamilyName;
  }

  /**
   * Get the name of the filter policy used in this table.
   *
   * @return the name of the filter policy, or null if
   *     no filter policy is used.
   */
  /*@Nullable*/ public String getFilterPolicyName() {
    return filterPolicyName;
  }

  /**
   * Get the name of the comparator used in this table.
   *
   * @return the name of the comparator.
   */
  public String getComparatorName() {
    return comparatorName;
  }

  /**
   * Get the name of the merge operator used in this table.
   *
   * @return the name of the merge operator, or null if no merge operator
   *      is used.
   */
  /*@Nullable*/ public String getMergeOperatorName() {
    return mergeOperatorName;
  }

  /**
   * Get the name of the prefix extractor used in this table.
   *
   * @return the name of the prefix extractor, or null if no prefix
   *     extractor is used.
   */
  /*@Nullable*/ public String getPrefixExtractorName() {
    return prefixExtractorName;
  }

  /**
   * Get the names of the property collectors factories used in this table.
   *
   * @return the names of the property collector factories separated
   *     by commas, e.g. {collector_name[1]},{collector_name[2]},...
   */
  public String getPropertyCollectorsNames() {
    return propertyCollectorsNames;
  }

  /**
   * Get the name of the compression algorithm used to compress the SST files.
   *
   * @return the name of the compression algorithm.
   */
  public String getCompressionName() {
    return compressionName;
  }

  /**
   * Get the user collected properties.
   *
   * @return the user collected properties.
   */
  public Map<String, String> getUserCollectedProperties() {
    return userCollectedProperties;
  }

  /**
   * Get the readable properties.
   *
   * @return the readable properties.
   */
  public Map<String, String> getReadableProperties() {
    return readableProperties;
  }

  @Override
  public boolean equals(final Object o) {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    final TableProperties that = (TableProperties) o;
    return dataSize == that.dataSize && indexSize == that.indexSize
        && indexPartitions == that.indexPartitions && topLevelIndexSize == that.topLevelIndexSize
        && indexKeyIsUserKey == that.indexKeyIsUserKey
        && indexValueIsDeltaEncoded == that.indexValueIsDeltaEncoded
        && filterSize == that.filterSize && rawKeySize == that.rawKeySize
        && rawValueSize == that.rawValueSize && numDataBlocks == that.numDataBlocks
        && numEntries == that.numEntries && numDeletions == that.numDeletions
        && numMergeOperands == that.numMergeOperands && numRangeDeletions == that.numRangeDeletions
        && formatVersion == that.formatVersion && fixedKeyLen == that.fixedKeyLen
        && columnFamilyId == that.columnFamilyId && creationTime == that.creationTime
        && oldestKeyTime == that.oldestKeyTime
        && slowCompressionEstimatedDataSize == that.slowCompressionEstimatedDataSize
        && fastCompressionEstimatedDataSize == that.fastCompressionEstimatedDataSize
        && externalSstFileGlobalSeqnoOffset == that.externalSstFileGlobalSeqnoOffset
        && Arrays.equals(columnFamilyName, that.columnFamilyName)
        && Objects.equals(filterPolicyName, that.filterPolicyName)
        && Objects.equals(comparatorName, that.comparatorName)
        && Objects.equals(mergeOperatorName, that.mergeOperatorName)
        && Objects.equals(prefixExtractorName, that.prefixExtractorName)
        && Objects.equals(propertyCollectorsNames, that.propertyCollectorsNames)
        && Objects.equals(compressionName, that.compressionName)
        && Objects.equals(userCollectedProperties, that.userCollectedProperties)
        && Objects.equals(readableProperties, that.readableProperties);
  }

  @Override
  public int hashCode() {
    int result = Objects.hash(dataSize, indexSize, indexPartitions, topLevelIndexSize,
        indexKeyIsUserKey, indexValueIsDeltaEncoded, filterSize, rawKeySize, rawValueSize,
        numDataBlocks, numEntries, numDeletions, numMergeOperands, numRangeDeletions, formatVersion,
        fixedKeyLen, columnFamilyId, creationTime, oldestKeyTime, slowCompressionEstimatedDataSize,
        fastCompressionEstimatedDataSize, externalSstFileGlobalSeqnoOffset, filterPolicyName,
        comparatorName, mergeOperatorName, prefixExtractorName, propertyCollectorsNames,
        compressionName, userCollectedProperties, readableProperties);
    result = 31 * result + Arrays.hashCode(columnFamilyName);
    return result;
  }
}
