package com.kivo.cinema.sources.access

import com.kivo.cinema.sources.SourceRecord

/** Filter BEFORE issuing requests; unlocking never opts hidden sources into aggregation. */
object SourceEligibility {
    fun visible(record: SourceRecord, unlocked: Boolean) = !record.hidden || unlocked
    fun home(records: List<SourceRecord>) = records.filter { !it.hidden && it.aggregateEnabled }
    fun search(records: List<SourceRecord>) = records.filter { !it.hidden && it.aggregateSearchEnabled }
}
