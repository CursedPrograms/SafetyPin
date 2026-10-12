// Safety Pin - the zine bits that need a bit of randomness (always the same randomness, seeded)
// and the toys: turnable knobs, the OVERKILL stamp and a tuner that tunes itself.

"use strict";

// Small seeded random generator (mulberry32), so torn edges and ransom letters never change
function seeded(seed) {
  let a = seed >>> 0;
  return function () {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

// ---------------------------------------------------------------- torn paper edges

function tornPolygon(rand, jagPercent) {
  const points = [];
  const jag = () => (rand() * jagPercent).toFixed(2);
  for (let x = 0; x < 100; x += 2 + rand() * 3) points.push(`${x.toFixed(2)}% ${jag()}%`);
  points.push(`100% ${jag()}%`);
  for (let y = 0; y < 100; y += 6 + rand() * 8) points.push(`${(100 - rand() * jagPercent * 0.4).toFixed(2)}% ${y.toFixed(2)}%`);
  for (let x = 100; x > 0; x -= 2 + rand() * 3) points.push(`${x.toFixed(2)}% ${(100 - jag()).toFixed(2)}%`);
  points.push(`0% ${(100 - jag()).toFixed(2)}%`);
  for (let y = 100; y > 0; y -= 6 + rand() * 8) points.push(`${(rand() * jagPercent * 0.4).toFixed(2)}% ${y.toFixed(2)}%`);
  return `polygon(${points.join(", ")})`;
}

document.querySelectorAll(".torn").forEach((el, i) => {
  const rand = seeded(Number(el.dataset.seed || i + 1));
  const tall = el.offsetHeight > 80;
  el.style.setProperty("--torn", tornPolygon(rand, tall ? 3 : 9));
});

// ---------------------------------------------------------------- ransom note letters

const RANSOM_FONTS = [
  '"Anton", Impact, sans-serif',
  '"Abril Fatface", Georgia, serif',
  '"Special Elite", "Courier New", monospace',
  '"Rubik Mono One", "Arial Black", sans-serif',
  'Georgia, "Times New Roman", serif',
  '"Courier Prime", "Courier New", monospace',
];
const RANSOM_BACKS = ["#f7f3e8", "#121212", "#f2e500", "#d9d4c7", "#f7f3e8", "#8cff3a"];

document.querySelectorAll("[data-ransom]").forEach((el) => {
  const rand = seeded(Number(el.dataset.seed || 1));
  el.textContent = "";
  for (const text of el.dataset.ransom.split(" ")) {
    const word = document.createElement("span");          // a word never breaks across lines
    word.className = "word";
    word.setAttribute("aria-hidden", "true");
    for (const ch of text) {
      const back = RANSOM_BACKS[Math.floor(rand() * RANSOM_BACKS.length)];
      const span = document.createElement("span");
      span.className = "cut";
      span.textContent = ch;
      span.style.fontFamily = RANSOM_FONTS[Math.floor(rand() * RANSOM_FONTS.length)];
      span.style.background = back;
      span.style.color = back === "#121212" ? (rand() < 0.5 ? "#ff2d8a" : "#f7f3e8") : (rand() < 0.3 ? "#e8112d" : "#121212");
      span.style.transform = `rotate(${((rand() - 0.5) * 18).toFixed(1)}deg) translateY(${((rand() - 0.5) * 10).toFixed(1)}px)`;
      word.appendChild(span);
    }
    el.appendChild(word);
  }
});

// ---------------------------------------------------------------- knobs

const START = -135, RANGE = 270;              // same sweep as the plugin's knobs

function buildKnob(knob, value) {
  const rand = seeded(knob.getAttribute("aria-label").length * 97);
  for (let i = 0; i <= 10; i++) {
    const tick = document.createElement("span");
    tick.className = i % 5 === 0 ? "tick big" : "tick";
    tick.style.transform = `rotate(${START + (RANGE * i) / 10 + (rand() - 0.5) * 3}deg)`;
    knob.appendChild(tick);
  }
  const ring = document.createElement("span");
  ring.className = "ring";
  for (let i = 0; i < 10; i++) {
    const a = (i / 10) * Math.PI * 2;
    const stud = document.createElement("span");
    stud.className = "stud";
    stud.style.left = `${50 + Math.sin(a) * 42}%`;
    stud.style.top = `${50 - Math.cos(a) * 42}%`;
    ring.appendChild(stud);
  }
  knob.appendChild(ring);
  const badge = document.createElement("span");
  badge.className = "badge";
  knob.appendChild(badge);
  const readout = document.createElement("span");
  readout.className = "readout";
  knob.appendChild(readout);

  knob.setAttribute("aria-valuemin", "0");
  knob.setAttribute("aria-valuemax", "10");
  setKnob(knob, value);
}

function setKnob(knob, value) {
  value = Math.min(1, Math.max(0, value));
  knob.dataset.value = value;
  knob.style.setProperty("--angle", `${START + value * RANGE}deg`);
  const shown = (value * 10).toFixed(1);
  knob.querySelector(".readout").textContent = shown;
  knob.setAttribute("aria-valuenow", shown);
}

document.querySelectorAll(".knob-card").forEach((card) => {
  const knob = card.querySelector(".knob");
  buildKnob(knob, Number(card.dataset.value || 0.5));

  let startY = 0, startValue = 0;
  knob.addEventListener("pointerdown", (e) => {
    startY = e.clientY;
    startValue = Number(knob.dataset.value);
    knob.setPointerCapture(e.pointerId);
    knob.classList.add("turning");
  });
  knob.addEventListener("pointermove", (e) => {
    if (!knob.hasPointerCapture(e.pointerId)) return;
    setKnob(knob, startValue + (startY - e.clientY) / 180);
  });
  const stop = () => knob.classList.remove("turning");
  knob.addEventListener("pointerup", stop);
  knob.addEventListener("pointercancel", stop);
  knob.addEventListener("wheel", (e) => {
    e.preventDefault();
    setKnob(knob, Number(knob.dataset.value) - Math.sign(e.deltaY) * 0.05);
  }, { passive: false });
  knob.addEventListener("keydown", (e) => {
    const step = { ArrowUp: 0.05, ArrowRight: 0.05, ArrowDown: -0.05, ArrowLeft: -0.05 }[e.key];
    if (step === undefined) return;
    e.preventDefault();
    setKnob(knob, Number(knob.dataset.value) + step);
  });
});

// ---------------------------------------------------------------- OVERKILL

const overkill = document.querySelector(".overkill");
overkill.addEventListener("click", () => {
  const on = overkill.getAttribute("aria-pressed") !== "true";
  overkill.setAttribute("aria-pressed", String(on));
  document.body.classList.toggle("overkill-on", on);
  if (on) setTimeout(() => overkill.getAttribute("aria-pressed") === "true" && overkill.click(), 4000);
});

// ---------------------------------------------------------------- the tuner tunes itself
// A string starts out of tune and gets turned towards pitch, then the next string, forever.

const tunerCard = document.querySelector(".tuner-card");
const needle = tunerCard.querySelector(".needle");
const noteEl = tunerCard.querySelector(".note");
const octaveEl = tunerCard.querySelector(".octave");
const strings = [...tunerCard.querySelectorAll(".tuner-strings span")];
const VIOLIN = [["G", 3], ["D", 4], ["A", 4], ["E", 5]];
const tunerRand = seeded(440);
let stringIndex = 2, cents = 0, target = 0, steps = 0;

function nextString() {
  stringIndex = (stringIndex + 1) % 4;
  cents = (tunerRand() < 0.5 ? -1 : 1) * (18 + tunerRand() * 28);
  steps = 0;
  noteEl.textContent = VIOLIN[stringIndex][0];
  octaveEl.textContent = VIOLIN[stringIndex][1];
  strings.forEach((s, i) => s.classList.toggle("lit", i === stringIndex));
}

function tick() {
  steps++;
  target = cents * 0.72;                                   // turning the peg
  cents = Math.abs(target) < 1 ? 0 : target;
  const wobble = cents === 0 ? (tunerRand() - 0.5) * 1.2 : (tunerRand() - 0.5) * 4;
  needle.style.setProperty("--cents", (cents + wobble).toFixed(1));
  tunerCard.classList.toggle("in-tune", Math.abs(cents) < 3);
  if (cents === 0 && steps > 14) nextString();
}

if (!window.matchMedia("(prefers-reduced-motion: reduce)").matches) {
  nextString();
  setInterval(tick, 350);
}
