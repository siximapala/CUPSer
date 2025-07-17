# CUPS-Logs-Parser
Utility in C++ for searching and parsing CUPS print log files.  
---

## 1. Description

CUPS stores its logs in the `/var/log/cups/` directory, where three files are typically found:

1. `access_log` — all HTTP requests to the CUPS daemon.
2. `error_log` — system messages and CUPS errors.
3. `page_log` — page-by-page information for each print job (printer name, user, number of pages, etc.).

Each of these files is plain text, with lines appended as events occur.

`cups_parser` recursively traverses the `<sysroot>/var/log/cups` directory (where `<sysroot>` defaults to `/`, or any other path specified via `-p`/`--path`), finds `access_log`, `error_log`, and `page_log`, and outputs their contents in one of two formats:
- **CSV**: a table with columns `File,Content` — each log line goes into the `Content` field, with line breaks replaced by `;`.
- **JSON**: an array of objects `{ "file": "...", "content": "..." }`.

If none of the three files is found, the program returns a non-zero exit code and an error message.

---

## 2. Installation (Ubuntu)

1. Install the compiler and tools:
   ```
   sudo apt install -y build-essential
   ```
2. Copy the project files into a single folder:
   ```
   main.cpp
   parser.cpp
   parser.h
   ```
3. Compile:
   ```
   g++ -std=c++17 main.cpp parser.cpp -o cups_parser
   ```
   This produces the `cups_parser` executable.

## 3. Usage and Parameters

```
./cups_parser [csv|json] [-p|--path <system_root>]
```
- `csv` or `json` — **required** parameter specifying the output format.
- `-p <system_root>` or `--path <system_root>` — optional: an alternative root from which to search for the `var/log/cups` directory. When used, the specified root path is prepended to the search path. Defaults to `/`.

### Examples

1. Search in the system root, output in CSV:
   ```
   ./cups_parser csv
   ```

2. Search in a test directory `/tmp/test-cups`, output in JSON:
   ```
   ./cups_parser json -p /tmp/test-cups
   ```
   (i.e., search in `/tmp/test-cups/var/log/cups`)

## 4. Exit Codes

0 — Success, data written to stdout  
1 — Invalid or incomplete argument  
2 — Output format not specified (csv/json)  
3 — CUPS logs not found in `<sysroot>/var/log/cups`

## 5. Project Structure

```
├── main.cpp      # entry point, argument parsing
├── parser.h      # declaration of parse_cups_logs
└── parser.cpp    # implementation of searching, reading, and formatting
```

## Additional: CUPS Log Structure

Three files in the `/var/log/cups` directory:

### 1. access_log

Records all HTTP requests to the CUPS daemon. Line format per documentation:

**`host group user date-time "method resource version" status bytes ipp-operation ipp-status`**

1. **host** — the client IP address that made the request to the print server.
2. **group** — the user group of the requester (Linux groups manage permissions).
3. **user** — the username initiating the request.
4. **date-time** — date and time of the request.
5. **"method resource version"** — the HTTP request, where:
   - *method* — HTTP method (`GET`, `HEAD`, `OPTIONS`, `POST`, `PUT`),
   - *resource* — requested path (e.g., `/printers/HP_LaserJet`),
   - *version* — protocol version (e.g., `HTTP/1.0`).
   - *status* — HTTP status code (e.g., `200` for OK).
   - *bytes* — number of bytes in the response.
6. **ipp-operation** — the IPP operation initiated (e.g., `Print-Job`).
7. **ipp-status** — the result of the IPP operation (e.g., `successful-ok`).

#### Parsing a log line:

`10.0.0.23 gruppa1 alice [22/May/2025:09:30:15 +0300] "GET /printers/Printer13HP HTTP/2.0" 304 0 Get-Printer-Attributes successful-ok`

- **IP** — 10.0.0.23  
- **Group** — gruppa1  
- **User** — alice  
- **Timestamp** — [22/May/2025:09:30:15 +0300]  
- **HTTP Method** — GET  
- **Resource** — `/printers/Printer13HP`  
- **Protocol Version** — HTTP/2.0  
- **Status** — 304  
- **Bytes** — 0  
- **IPP Operation** — Get-Printer-Attributes  
- **IPP Status** — successful-ok  

### 2. error_log

Contains scheduler messages — errors, warnings, etc. Line format:

**`level [date-time] message`**

1. **level** — log level:
   - **A** — alert  
   - **C** — crit  
   - **D** — debug  
   - **d** — debug2  
   - **E** — error  
   - **I** — info  
   - **N** — notice  
   - **W** — warn  
   - **X** — emerg  
2. **date-time** — date and time (same format as `access_log`).
3. **message** — free-form text. Job filter messages include `[Job NNN]` prefix.

**Job ID** is a unique numeric identifier for each print job.

#### Parsing an error log line:

`I [24/May/2025:11:15:05 +0300] [Job 58] Job cancelled by nikita`

- **Level** — I (info)  
- **Timestamp** — [24/May/2025:11:15:05 +0300]  
- **Job Tag** — Job 58  
- **Message** — Job cancelled by nikita  

### 3. page_log

Records details about each printed page (or group of pages) per job.

#### Line format:

`printer user job-id date-time page-number num-copies job-billing job-originating-host-name job-name media sides`

- **printer** — the printer that printed the page.  
- **user** — the user who sent the job.  
- **job-id** — the unique print job identifier.  
- **date-time** — print start time (same format as above).  
- **page-number** — the page number within the job.  
- **num-copies** — number of copies of that page.  
- **job-billing** — IPP `job-billing` or `job-account-id` (or `-` if unset).  
- **job-originating-host-name** — hostname or IP of the client.  
- **job-name** — the job name (or `-` if unset).  
- **media** — paper size (or `-` if unset).  
- **sides** — one-sided, two-sided, etc.  

If `total` appears instead of `page-number` and `num-copies`, it indicates **num-impressions** — total sides printed for the entire job.

#### Parsing a page_log line:

`DeskJet root 1 [20/May/1999:19:21:05 +0000] 1 1 acme-123 localhost myjob iso_a4_210x297mm one-sided`

1. **printer**: DeskJet  
2. **user**: root  
3. **job-id**: 1  
4. **date-time**: [20/May/1999:19:21:05 +0000]  
5. **page-number**: 1  
6. **num-copies**: 1  
7. **job-billing**: acme-123  
8. **job-originating-host-name**: localhost  
9. **job-name**: myjob  
10. **media**: iso_a4_210x297mm  
11. **sides**: one-sided  
