package com.zedensix.gt86dash

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.provider.DocumentsContract
import android.text.Editable
import android.text.InputType
import android.text.TextWatcher
import android.view.View
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.TextView
import java.text.DateFormat
import java.util.Date

// The settings page's column for what the phone records: the dash's log
// pulled every minute, its backup (off, a folder on the phone, a WebDAV
// folder). The folder's picker answers through the activity
// (folderPicked). Lap timing and spirited drives have their own pages.
class RecordingSettings(private val activity: Activity, private val label: (String) -> View, private val help: (String) -> View) {
    private val settings = Settings(activity)
    private lateinit var pulled: TextView
    private lateinit var kinds: List<Pair<Backup.Kind, TextView>>
    private lateinit var folder: View
    private lateinit var folderName: TextView
    private lateinit var webdav: View
    private lateinit var backupResult: TextView

    fun build(): View {
        val a = activity
        val c = LinearLayout(a).apply { orientation = LinearLayout.VERTICAL }
        c.addView(label("DATA LOG"))
        c.addView(pageSwitch(a, "Pull the dash's log every minute", settings.collect) { settings.collect = it })
        pulled = text("", 16f, Colors.DIM)
        c.addView(pulled)

        c.addView(label("BACKUP"))
        val row = LinearLayout(a)
        kinds = Backup.Kind.entries.map { kind ->
            val pill = pagePill(a, kind.title) {
                settings.backup = kind
                refresh()
            }
            row.addView(pill, LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f).apply { marginEnd = dp(a, 6) })
            kind to pill
        }
        c.addView(row)
        folderName = text("", 18f, Colors.LABEL)
        folder = LinearLayout(a).apply {
            orientation = LinearLayout.VERTICAL
            addView(folderName)
            addView(pagePill(a, "Choose the folder") { activity.startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), FOLDER_REQUEST) })
        }
        c.addView(folder, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(a, 8) })
        webdav = LinearLayout(a).apply {
            orientation = LinearLayout.VERTICAL
            addView(field("https://files.example.com/dav/me/GT86/", settings.webdavUrl, InputType.TYPE_TEXT_VARIATION_URI) { settings.webdavUrl = it.trim() })
            addView(field("API key", settings.webdavKey, InputType.TYPE_TEXT_VARIATION_PASSWORD) { settings.webdavKey = it.trim() })
            addView(help("The folder must exist. Penombre: Settings, API keys"))
        }
        c.addView(webdav, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(a, 8) })
        c.addView(pagePill(a, "Back up now") { Backup.run(a) }, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(a, 8) })
        backupResult = text("", 16f, Colors.DIM)
        c.addView(backupResult)
        c.addView(help("Each session's log as CSV, each drive as GPX, again as they grow"))

        refresh()
        return c
    }

    fun folderPicked(uri: Uri?) {
        uri ?: return
        activity.contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
        settings.folder = uri.toString()
        settings.folderName = DocumentsContract.getTreeDocumentId(uri).substringAfter(':').ifEmpty { "the storage's root" }
        refresh()
    }

    fun refresh() {
        val at = Logs.pulledAt(activity)
        val count = Logs.sessions(activity).size
        pulled.text = (if (at == 0L) "Never pulled yet" else "Last pull at ${DateFormat.getTimeInstance(DateFormat.SHORT).format(Date(at))}") +
            ", $count session${if (count == 1) "" else "s"} on the phone"
        kinds.forEach { (kind, pill) -> pill.setBackgroundResource(if (kind == settings.backup) R.drawable.pill_selected else R.drawable.pill_button) }
        folder.visibility = if (settings.backup == Backup.Kind.FOLDER) View.VISIBLE else View.GONE
        folderName.text = settings.folderName?.let { "Into $it" } ?: "No folder chosen"
        webdav.visibility = if (settings.backup == Backup.Kind.WEBDAV) View.VISIBLE else View.GONE
        backupResult.text = Backup.result
    }

    private fun text(s: String, size: Float, color: Int) = pageText(activity, s, size, R.font.barlow_condensed_medium, color)

    private fun field(hint: String, value: String, variation: Int, changed: (String) -> Unit) =
        EditText(activity).apply {
            setText(value)
            this.hint = hint
            textSize = 18f
            typeface = activity.resources.getFont(R.font.barlow_condensed_medium)
            setTextColor(Colors.LABEL)
            setHintTextColor(Colors.DIM)
            inputType = InputType.TYPE_CLASS_TEXT or variation
            isSingleLine = true
            addTextChangedListener(
                object : TextWatcher {
                    override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}

                    override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}

                    override fun afterTextChanged(s: Editable?) = changed(s?.toString() ?: "")
                },
            )
        }

    companion object {
        const val FOLDER_REQUEST = 10
    }
}
