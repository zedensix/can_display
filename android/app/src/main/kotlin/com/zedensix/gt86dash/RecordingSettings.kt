package com.zedensix.gt86dash

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.DocumentsContract
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts.OpenDocumentTree
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import java.text.DateFormat
import java.util.Date

// The settings' column for what the phone records: the dash's log pulled
// every minute, its backup (off, a folder on the phone, a WebDAV folder).
// A folder picked (the system's picker) is kept for good and tested
// (Backup.test). Lap timing and spirited drives have their own pages.
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun RecordingSection() {
    val context = LocalContext.current
    val settings = remember { Settings(context) }
    ticking(TICK_MS)
    var collect by remember { mutableStateOf(settings.collect) }
    var backup by remember { mutableStateOf(settings.backup) }
    var folder by remember { mutableStateOf(settings.folderName) }
    var url by remember { mutableStateOf(settings.webdavUrl) }
    var key by remember { mutableStateOf(settings.webdavKey) }
    val pick = rememberLauncherForActivityResult(OpenDocumentTree()) { uri ->
        if (uri != null) {
            keepFolder(context, settings, uri)
            folder = settings.folderName
        }
    }

    val at = Logs.pulledAt(context)
    val count = Logs.sessions(context).size
    val pulled = (if (at == 0L) "Never pulled yet" else "Last pull at ${DateFormat.getTimeInstance(DateFormat.SHORT).format(Date(at))}") +
        ", $count session${if (count == 1) "" else "s"} on the phone"
    SectionTitle("Data log")
    SectionCard {
        Item(
            "Pull the dash's log every minute",
            pulled,
            trailing = {
                Switch(checked = collect, onCheckedChange = {
                    collect = it
                    settings.collect = it
                })
            },
            onClick = {
                collect = !collect
                settings.collect = collect
            },
        )
    }

    SectionTitle("Backup")
    SectionCard {
        SingleChoiceSegmentedButtonRow(Modifier.fillMaxWidth().padding(16.dp)) {
            Backup.Kind.entries.forEachIndexed { i, kind ->
                SegmentedButton(
                    selected = backup == kind,
                    onClick = {
                        backup = kind
                        settings.backup = kind
                    },
                    shape = SegmentedButtonDefaults.itemShape(i, Backup.Kind.entries.size),
                    icon = {},
                ) { Text(kind.title, maxLines = 1) }
            }
        }
        if (backup == Backup.Kind.FOLDER) {
            Divider()
            Item("Folder", folder?.let { "Into $it" } ?: "None chosen", trailing = { Chevron() }, onClick = { pick.launch(null) })
        }
        if (backup == Backup.Kind.WEBDAV) {
            OutlinedTextField(
                value = url,
                onValueChange = {
                    url = it
                    settings.webdavUrl = it.trim()
                },
                label = { Text("WebDAV folder") },
                placeholder = { Text("https://files.example.com/dav/me/GT86/") },
                singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Uri),
                modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp),
            )
            OutlinedTextField(
                value = key,
                onValueChange = {
                    key = it
                    settings.webdavKey = it.trim()
                },
                label = { Text("API key") },
                singleLine = true,
                visualTransformation = PasswordVisualTransformation(),
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password),
                modifier = Modifier.fillMaxWidth().padding(start = 16.dp, end = 16.dp, top = 8.dp, bottom = 8.dp),
            )
        }
        if (backup != Backup.Kind.OFF) {
            Divider()
            val result = Backup.result
            val failed = result.startsWith("Can't") || result.startsWith("Backup failed")
            Item("Back up now", result, supportingColor = if (failed) Color(Colors.DANGER) else Color.Unspecified, onClick = { Backup.run(context) })
        }
    }
    if (backup == Backup.Kind.WEBDAV) Footnote("The folder must exist. Penombre: Settings, API keys")
    Footnote("Each session's log as CSV, each drive as GPX, again as they grow")
}

// The folder the system's picker answered: allowed for good, kept, tested.
private fun keepFolder(context: Context, settings: Settings, uri: Uri) {
    context.contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
    settings.folder = uri.toString()
    settings.folderName = DocumentsContract.getTreeDocumentId(uri).substringAfter(':').ifEmpty { "the storage's root" }
    Backup.test(context)
}

private const val TICK_MS = 1000L
