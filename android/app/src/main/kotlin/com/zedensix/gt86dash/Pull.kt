package com.zedensix.gt86dash

import java.io.ByteArrayOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

// A pull's header, as the dash's ble.c sends it: how many bytes follow,
// where they start in its flash, where its log is now (the next pull's
// from), its milliseconds since boot and its session.
class PullHeader(val total: Int, val start: Long, val next: Long, val nowMs: Long, val session: Int) {
    companion object {
        const val SIZE = 18

        fun decode(b: ByteArray): PullHeader? {
            if (b.size < SIZE) return null
            val buf = ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN)
            fun u32(i: Int) = buf.getInt(i).toLong() and 0xFFFFFFFFL
            return PullHeader(u32(0).toInt(), u32(4), u32(8), u32(12), buf.getShort(16).toInt() and 0xFFFF)
        }
    }
}

// What the dash logged since the last pull: asked with 'R' and where the
// last one ended (FROM_OLDEST the first time), it comes as notifications
// of its data characteristic, the header, then the bytes. heardAt: when
// it was asked for, then when its last notification came (uptime ms).
class Pull(var heardAt: Long, private val done: (PullHeader, ByteArray) -> Unit) {
    private var header: PullHeader? = null
    private val bytes = ByteArrayOutputStream()

    // Bytes in and how many come, once the header said.
    val got: Int get() = bytes.size()
    val total: Int? get() = header?.total

    // Takes a notification at now; returns whether the pull is over. A
    // header saying more than the dash's log holds isn't one: given up.
    fun take(value: ByteArray, now: Long): Boolean {
        heardAt = now
        val h = header
        if (h == null) {
            header = PullHeader.decode(value)?.takeIf { it.total in 0..MAX_BYTES } ?: return true
        } else {
            bytes.write(value)
        }
        val got = header!!
        if (bytes.size() < got.total) return false
        done(got, bytes.toByteArray().copyOf(got.total))
        return true
    }

    companion object {
        const val FROM_OLDEST = 0xFFFFFFFFL

        // A pull with nothing for this long is given up: the dash streams
        // one without a break.
        const val QUIET_MS = 15_000L

        // The dash's data log is under a megabyte.
        const val MAX_BYTES = 2 shl 20

        // forUpdate: the dash shows it, the update's screen up from then on.
        fun request(from: Long, forUpdate: Boolean): ByteArray =
            ByteBuffer.allocate(6).order(ByteOrder.LITTLE_ENDIAN).put('R'.code.toByte()).putInt(from.toInt())
                .put(if (forUpdate) 1 else 0).array()
    }
}
