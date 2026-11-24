package com.zedensix.gt86dash

import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract
import java.net.HttpURLConnection
import java.net.URI
import java.net.URLEncoder
import java.text.DateFormat
import java.util.Base64
import java.util.Date
import java.util.concurrent.Executors

// The logs and drives copied off the phone's own storage, each into one
// flat folder: a folder on the phone picked in the settings (the system's
// picker: internal storage, an SD card), or a WebDAV folder (Penombre's,
// with an API key as the password; it must exist: WebDAV won't make it on
// a PUT). Each session's log as CSV, each drive as GPX, again whenever it
// grew; run on its own thread, its outcome in result.
object Backup {
    enum class Kind(val title: String) { OFF("Off"), FOLDER("Folder"), WEBDAV("WebDAV") }

    @Volatile var result: String = ""
        private set

    private val worker = Executors.newSingleThreadExecutor()

    fun run(c: Context) {
        val app = c.applicationContext
        worker.execute {
            result = try {
                val n = backUp(app)
                if (n < 0) "" else "Backed up $n file${if (n == 1) "" else "s"}, ${DateFormat.getTimeInstance(DateFormat.SHORT).format(Date())}"
            } catch (e: Exception) {
                "Backup failed: ${e.message}"
            }
        }
    }

    // What changed since its last backup there, copied; how many (-1: off).
    private fun backUp(c: Context): Int {
        val settings = Settings(c)
        val kind = settings.backup
        if (kind == Kind.OFF) return -1
        val done = c.getSharedPreferences("backups", Context.MODE_PRIVATE)
        val lines = settings.startLines
        val files = Logs.sessions(c).map { s ->
            Triple(s.backupName, s.file) { Records.csv(s.samples(), s.bootAt).toByteArray() }
        } + Drives.all(c).map { d ->
            Triple(d.backupName, d.file) { Drives.gpx(DriveSummary.of(d, lines).fixes).toByteArray() }
        }
        var n = 0
        for ((name, source, bytes) in files) {
            val version = "${source.length()}:${source.lastModified()}"
            val key = "${kind.name}:$name"
            if (done.getString(key, null) == version) continue
            when (kind) {
                Kind.FOLDER -> toFolder(c, settings.folder ?: throw Exception("no folder picked"), name, bytes())
                Kind.WEBDAV -> toWebdav(settings.webdavUrl, settings.webdavKey, name, bytes())
                Kind.OFF -> return -1
            }
            done.edit().putString(key, version).apply()
            n++
        }
        return n
    }

    // Into the folder picked (a document tree), over the file of that
    // name if there's one.
    private fun toFolder(c: Context, uri: String, name: String, bytes: ByteArray) {
        val tree = Uri.parse(uri)
        val root = DocumentsContract.getTreeDocumentId(tree)
        val resolver = c.contentResolver
        val children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, root)
        val columns = arrayOf(DocumentsContract.Document.COLUMN_DOCUMENT_ID, DocumentsContract.Document.COLUMN_DISPLAY_NAME)
        val existing = resolver.query(children, columns, null, null, null)?.use { cursor ->
            generateSequence { if (cursor.moveToNext()) cursor.getString(0) to cursor.getString(1) else null }
                .firstOrNull { it.second == name }?.first
        }
        val target = existing?.let { DocumentsContract.buildDocumentUriUsingTree(tree, it) }
            ?: DocumentsContract.createDocument(resolver, DocumentsContract.buildDocumentUriUsingTree(tree, root), "application/octet-stream", name)
            ?: throw Exception("the folder refused $name")
        (resolver.openOutputStream(target, "wt") ?: throw Exception("can't write $name")).use { it.write(bytes) }
    }

    private fun toWebdav(base: String, key: String, name: String, bytes: ByteArray) {
        if (!base.startsWith("https://")) throw Exception("the WebDAV address must start with https://")
        val url = URI(base.trimEnd('/') + "/" + URLEncoder.encode(name, "UTF-8")).toURL()
        val http = url.openConnection() as HttpURLConnection
        try {
            http.requestMethod = "PUT"
            http.doOutput = true
            http.connectTimeout = 15_000
            http.readTimeout = 60_000
            http.setFixedLengthStreamingMode(bytes.size)
            http.setRequestProperty("Authorization", "Basic " + Base64.getEncoder().encodeToString("gt86:$key".toByteArray()))
            http.outputStream.use { it.write(bytes) }
            when (val code = http.responseCode) {
                in 200..299 -> return
                401, 403 -> throw Exception("the key was refused")
                409 -> throw Exception("no such folder: make it on the server first")
                else -> throw Exception("the server answered $code")
            }
        } finally {
            http.disconnect()
        }
    }
}
