package com.zedensix.gt86dash

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import java.nio.ByteBuffer
import java.nio.ByteOrder

// The data log's records as datalog.c writes them, a pull's header as
// ble.c sends it, and the laps on a start line.
class RecorderTest {
    private fun record(session: Int, ms: Int, rpm: Int): ByteArray {
        val b = ByteBuffer.allocate(Records.RECORD).order(ByteOrder.LITTLE_ENDIAN)
        repeat(Records.RECORD) { b.put(it, 0xFF.toByte()) }
        b.putShort(0, session.toShort()).put(2, 2).putInt(4, ms)
        Records.COLUMNS.indices.forEach { b.putShort(8 + 2 * it, (-32768).toShort()) }
        b.putShort(8, rpm.toShort())
        return b.array()
    }

    @Test
    fun walksRecordsPastErasedAndOldSlots() {
        val erased = ByteArray(Records.SLOT) { 0xFF.toByte() }
        val old = ByteArray(Records.SLOT).also { it[2] = 1 }
        val samples = Records.parse(erased + old + record(7, 1200, 3000) + record(7, 1400, 3100))
        assertEquals(2, samples.size)
        assertEquals(7, samples[0].session)
        assertEquals(1400L, samples[1].ms)
        assertEquals(3100f, samples[1].values[0]!!, 0f)
        assertNull(samples[0].values[1])
    }

    @Test
    fun csvHasDotsWhateverTheLanguage() {
        val csv = Records.csv(Records.parse(record(3, 1500, 2500)), null)
        assertEquals(",3,1.5,2500" + ",".repeat(Records.COLUMNS.size - 1), csv.lines()[1])
    }

    @Test
    fun decodesAPullHeader() {
        val b = ByteBuffer.allocate(PullHeader.SIZE).order(ByteOrder.LITTLE_ENDIAN)
            .putInt(128).putInt(0x1000).putInt(0x1080).putInt(-1).putShort(42).array()
        val h = PullHeader.decode(b)!!
        assertEquals(128, h.total)
        assertEquals(0x1080L, h.next)
        assertEquals(0xFFFFFFFFL, h.nowMs)
        assertEquals(42, h.session)
        assertNull(PullHeader.decode(ByteArray(4)))
    }

    // 'R', where from (little-endian), then 1 for the pull before an
    // update (ble.c: the dash shows it).
    @Test
    fun asksAPull() {
        assertEquals(listOf(0x52, 0x80, 0x00, 0x01, 0x00, 0x00), Pull.request(0x10080, false).map { it.toInt() and 0xFF })
        assertEquals(1.toByte(), Pull.request(Pull.FROM_OLDEST, true)[5])
    }

    @Test
    fun pullTakesTheHeaderThenTheBytes() {
        var got: ByteArray? = null
        val pull = Pull(0) { _, bytes -> got = bytes }
        val header = ByteBuffer.allocate(PullHeader.SIZE).order(ByteOrder.LITTLE_ENDIAN).putInt(5).array()
        assertEquals(false, pull.take(header, 1))
        assertEquals(false, pull.take(byteArrayOf(1, 2, 3), 2))
        assertEquals(true, pull.take(byteArrayOf(4, 5), 3))
        assertEquals(3L, pull.heardAt)
        assertEquals(listOf<Byte>(1, 2, 3, 4, 5), got!!.toList())
    }

    // A line across a road running north, crossed northbound once a lap.
    private val line = StartLine(45.0, 3.0, 0f)
    private val degreesPerM = 1 / StartLine.M_PER_DEGREE

    private fun fix(ms: Long, northM: Double, eastM: Double = 0.0) =
        Fix(ms, 45.0 + northM * degreesPerM, 3.0 + eastM * degreesPerM / Math.cos(Math.toRadians(45.0)), 30f, 0f, 3f)

    @Test
    fun crossingIsInterpolatedBetweenFixes() {
        // 10 m short of the line, then 30 m past it, a second apart.
        assertEquals(1250.0, line.crossing(fix(1000, -10.0), fix(2000, 30.0))!!.toDouble(), 2.0)
        // The wrong way, too far to the side, or across a gap: none.
        assertNull(line.crossing(fix(1000, 30.0), fix(2000, -10.0)))
        assertNull(line.crossing(fix(1000, -10.0, 50.0), fix(2000, 30.0, 50.0)))
        assertNull(line.crossing(fix(1000, -10.0), fix(9000, 30.0)))
    }

    @Test
    fun lapsRunFromCrossingToCrossing() {
        val t = LapTimer(line)
        val crossings = listOf(10_000L, 95_000L, 178_500L)
        for (at in crossings) {
            t.add(fix(at - 500, -10.0))
            t.add(fix(at + 500, 10.0))
        }
        assertEquals(listOf(85_000L, 83_500L), t.laps)
        assertEquals(83_500L, t.best)
        assertEquals("1:23.5", LapTimer.text(83_500))
    }

    @Test
    fun aCrossingTooSoonIsGpsWandering() {
        val t = LapTimer(line)
        t.add(fix(9_500, -10.0))
        t.add(fix(10_500, 10.0))
        t.add(fix(11_500, -10.0))
        t.add(fix(12_500, 10.0))
        assertEquals(emptyList<Long>(), t.laps)
    }

    @Test
    fun startLinesSurviveTheSettings() {
        assertEquals(line, StartLine.decode(line.encode()))
        assertNull(StartLine.decode("nonsense"))
        assertEquals(line, StartLine.nearest(listOf(StartLine(46.0, 3.0, 0f), line), 45.001, 3.0))
    }
}
