// SmartLMS Client-Side Hardware Simulator & Visualizer

const INITIAL_BOOKS = [
    { isbn: "978-1593272203", rfid: "RFID-LPI-001", title: "The Linux Programming Interface", aisle: 1, rack: 1, shelf: 2, status: "AVAILABLE", esl: "green" },
    { isbn: "978-0201704310", rfid: "RFID-MCD-002", title: "Modern C++ Design", aisle: 1, rack: 2, shelf: 1, status: "AVAILABLE", esl: "green" },
    { isbn: "978-0596005900", rfid: "RFID-LDD-003", title: "Linux Device Drivers 3rd Ed", aisle: 1, rack: 3, shelf: 3, status: "AVAILABLE", esl: "green" },
    { isbn: "978-1985086593", rfid: "RFID-OSTEP-004", title: "Operating Systems: 3 Easy Pieces", aisle: 2, rack: 1, shelf: 1, status: "AVAILABLE", esl: "green" },
    { isbn: "978-0201633610", rfid: "RFID-GOF-005", title: "Design Patterns (GoF)", aisle: 2, rack: 2, shelf: 2, status: "AVAILABLE", esl: "green" },
    { isbn: "978-1491903995", rfid: "RFID-EMC-006", title: "Effective Modern C++", aisle: 1, rack: 2, shelf: 2, status: "AVAILABLE", esl: "green" },
    { isbn: "978-0134092669", rfid: "RFID-CSAPP-007", title: "Computer Systems: A Programmer's Perspective", aisle: 2, rack: 3, shelf: 1, status: "AVAILABLE", esl: "green" },
    { isbn: "978-0321637734", rfid: "RFID-APUE-008", title: "Adv. Programming in UNIX Environment", aisle: 1, rack: 1, shelf: 1, status: "AVAILABLE", esl: "green" }
];

let state = {
    books: JSON.parse(JSON.stringify(INITIAL_BOOKS)),
    activeLoans: 0,
    totalCheckouts: 0,
    totalReturns: 0,
    securityBreaches: 0,
    isSolenoidLocked: true,
    isAlarmActive: false,
    activeLoanIsbns: new Set()
};

// Web Audio API Alarm Synthesizer
let audioCtx = null;
function playAlarmBeep() {
    try {
        if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        const osc = audioCtx.createOscillator();
        const gain = audioCtx.createGain();
        osc.type = "sawtooth";
        osc.frequency.setValueAtTime(880, audioCtx.currentTime); // High pitch A5
        osc.frequency.setValueAtTime(659, audioCtx.currentTime + 0.15); // E5
        gain.gain.setValueAtTime(0.15, audioCtx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.01, audioCtx.currentTime + 0.35);
        osc.connect(gain);
        gain.connect(audioCtx.destination);
        osc.start();
        osc.stop(audioCtx.currentTime + 0.35);
    } catch (e) {
        console.warn("Audio unavailable:", e);
    }
}

function logEvent(tag, message) {
    const stream = document.getElementById("logStream");
    const entry = document.createElement("div");
    entry.className = "log-entry";

    const now = new Date().toISOString().replace('T', ' ').substring(11, 23);
    entry.innerHTML = `<span class="log-time">[${now}]</span> <span class="log-tag ${tag}">[${tag}]</span> <span>${message}</span>`;
    stream.prepend(entry);
}

function updateTelemetryView() {
    document.getElementById("statActiveLoans").textContent = state.activeLoans;
    document.getElementById("statCheckouts").textContent = state.totalCheckouts;
    document.getElementById("statReturns").textContent = state.totalReturns;
    document.getElementById("statBreaches").textContent = state.securityBreaches;

    const solText = document.getElementById("statSolenoid");
    const solBox = document.getElementById("solenoidText");
    if (state.isSolenoidLocked) {
        solText.textContent = "LOCKED";
        solBox.textContent = "LOCKED";
        document.getElementById("solenoidBox").style.borderColor = "var(--card-border)";
    } else {
        solText.textContent = "UNLOCKED";
        solBox.textContent = "UNLOCKED (Flap Open)";
        document.getElementById("solenoidBox").style.borderColor = "var(--accent-green)";
    }

    const gateBox = document.getElementById("gateBox");
    const gateText = document.getElementById("gateStatusText");
    if (state.isAlarmActive) {
        gateBox.classList.add("breach");
        gateText.textContent = "GATE STATUS: *** THEFT BREACH DETECTED ***";
    } else {
        gateBox.classList.remove("breach");
        gateText.textContent = "GATE STATUS: SECURE & NORMAL";
    }
}

function renderRacks() {
    const a1Grid = document.getElementById("aisle1Racks");
    const a2Grid = document.getElementById("aisle2Racks");
    a1Grid.innerHTML = "";
    a2Grid.innerHTML = "";

    state.books.forEach((book) => {
        const box = document.createElement("div");
        box.className = "rack-box";
        box.innerHTML = `
            <div class="esl-led led-${book.esl}"></div>
            <div class="rack-id">Rack ${book.rack} • Shelf ${book.shelf}</div>
            <div class="book-title-short" title="${book.title}">${book.title}</div>
            <div class="book-rfid">${book.rfid}</div>
        `;

        if (book.aisle === 1) {
            a1Grid.appendChild(box);
        } else {
            a2Grid.appendChild(box);
        }
    });

    populateSelects();
}

function populateSelects() {
    const chkSelect = document.getElementById("checkoutBookSelect");
    const retSelect = document.getElementById("returnBookSelect");

    chkSelect.innerHTML = "";
    retSelect.innerHTML = "";

    state.books.forEach(b => {
        if (b.status === "AVAILABLE") {
            const opt = document.createElement("option");
            opt.value = b.isbn;
            opt.textContent = `${b.title} (${b.rfid})`;
            chkSelect.appendChild(opt);
        } else if (b.status === "CHECKED_OUT") {
            const opt = document.createElement("option");
            opt.value = b.rfid;
            opt.textContent = `${b.title} (${b.rfid})`;
            retSelect.appendChild(opt);
        }
    });
}

// Handlers
document.getElementById("btnCheckout").addEventListener("click", () => {
    const isbn = document.getElementById("checkoutBookSelect").value;
    const user = document.getElementById("userSelect").value;
    if (!isbn) return;

    const book = state.books.find(b => b.isbn === isbn);
    if (book) {
        book.status = "CHECKED_OUT";
        book.esl = "grey";
        state.activeLoans++;
        state.totalCheckouts++;
        state.activeLoanIsbns.add(book.isbn);

        logEvent("INFO", `Kiosk Checkout: Scanned ${book.rfid} to patron ${user}. ESL set to DIM.`);
        updateTelemetryView();
        renderRacks();
    }
});

document.getElementById("btnDropReturn").addEventListener("click", () => {
    const rfid = document.getElementById("returnBookSelect").value;
    if (!rfid) return;

    const book = state.books.find(b => b.rfid === rfid);
    if (book) {
        // Unlock Solenoid
        state.isSolenoidLocked = false;
        updateTelemetryView();
        logEvent("INFO", `Drop-Box Chute: RFID ${rfid} detected. Actuating solenoid flap to UNLOCKED.`);

        setTimeout(() => {
            book.status = "AVAILABLE";
            book.esl = "amber"; // amber during intake/reshelving
            state.activeLoans = Math.max(0, state.activeLoans - 1);
            state.totalReturns++;
            state.activeLoanIsbns.delete(book.isbn);
            state.isSolenoidLocked = true;

            logEvent("INFO", `Drop-Box Chute: Book received into bin. Flap RELOCKED. Status -> RETURNED_DROPBOX.`);
            updateTelemetryView();
            renderRacks();
        }, 800);
    }
});

document.getElementById("btnTestBreach").addEventListener("click", () => {
    // Pick an unborrowed book
    const unborrowed = state.books.find(b => b.status === "AVAILABLE");
    const tag = unborrowed ? unborrowed.rfid : "RFID-UNKNOWN-999";
    const title = unborrowed ? unborrowed.title : "Unregistered Volume";

    state.isAlarmActive = true;
    state.securityBreaches++;
    playAlarmBeep();
    logEvent("ALERT", `EXIT GATE THEFT ALERT: Unauthorized RFID ${tag} ('${title}') detected passing gate!`);
    updateTelemetryView();
});

document.getElementById("btnTestAuthorized").addEventListener("click", () => {
    if (state.activeLoanIsbns.size === 0) {
        logEvent("WARN", `Exit Gate: No books are currently loaned out. Please check out a book first.`);
        return;
    }
    const loanedIsbn = Array.from(state.activeLoanIsbns)[0];
    const book = state.books.find(b => b.isbn === loanedIsbn);
    if (book) {
        logEvent("INFO", `EXIT GATE GREEN: Authorized departure confirmed for '${book.title}' (${book.rfid}).`);
    }
});

document.getElementById("btnSilenceAlarm").addEventListener("click", () => {
    state.isAlarmActive = false;
    logEvent("INFO", "Security Operator: Anti-theft gate siren silenced via IOCTL.");
    updateTelemetryView();
});

document.getElementById("btnClearLog").addEventListener("click", () => {
    document.getElementById("logStream").innerHTML = "";
});

// Initialization
logEvent("INFO", "SmartLMS Hardware Visualizer connected to mock HAL engine.");
renderRacks();
updateTelemetryView();
