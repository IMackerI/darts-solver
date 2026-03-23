/**
 * @file main.js
 * @brief App entry point — handles routing, WASM initialisation, and page lifecycle.
 */
import * as State from './state.js';
import * as Wasm  from './wasm.js';
import { parseTarget, computeBounds } from './target.js';
import * as CalibrationPage from './pages/calibration.js';
import * as SolverPage from './pages/solver.js';

let targetBeds = null;
let targetBounds = null;

/* ---------- landing page animations ---------- */

let landingActive = false;
let particlesInstance = null;

let dotAnimationId = null;
let mouse = { x: -1000, y: -1000 };
let scrollSpeed = 0;

async function initLandingAnimations() {
    if (landingActive) return;
    landingActive = true;

    await waitForLibs();

    // ── Custom Interactive Canvas Dots ──
    const canvas = document.getElementById('landing-particles');
    const content = document.getElementById('content');
    if (canvas && content) {
        const ctx = canvas.getContext('2d');
        let dots = [];
        const dotCount = 140;
        
        const resize = () => {
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;
        };
        window.addEventListener('resize', resize);
        resize();

        class Dot {
            constructor() {
                this.x = Math.random() * canvas.width;
                this.y = Math.random() * canvas.height;
                this.baseSize = Math.random() * 2 + 1;
                this.size = this.baseSize;
                this.vx = (Math.random() - 0.5) * 1.2;
                this.vy = (Math.random() - 0.5) * 1.2;
                this.accX = 0;
                this.accY = 0;
            }
            update() {
                // Mouse interaction (repulsion + slow down)
                const dx = mouse.x - this.x;
                const dy = mouse.y - this.y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                let speedFactor = 1.0;

                if (dist < 200) {
                    const force = (200 - dist) / 200;
                    // Muted repulsion (10x reduction)
                    this.accX -= dx * force * 0.0003; 
                    this.accY -= dy * force * 0.0003;
                    // SLOW DOWN: reduce existing velocity when near cursor
                    speedFactor = 0.3 + (dist / 200) * 0.7; 
                }

                this.vx += this.accX;
                this.vy += this.accY;
                this.accX *= 0.9;
                this.accY *= 0.9;
                if (this.vx + this.vy > 3) {
                    this.vx *= 0.999;
                    this.vy *= 0.999;
                }

                // Scroll interaction (speed up)
                const scrollFactor = 1 + scrollSpeed * 0.15;
                this.x += this.vx * speedFactor * scrollFactor;
                this.y += this.vy * speedFactor * scrollFactor;

                if (this.x < 0 || this.x > canvas.width) this.vx *= -1;
                if (this.y < 0 || this.y > canvas.height) this.vy *= -1;
            }
            draw() {
                ctx.fillStyle = 'rgba(78, 201, 176, 0.45)';
                ctx.beginPath();
                ctx.arc(this.x, this.y, this.size, 0, Math.PI * 2);
                ctx.fill();
            }
        }

        for (let i = 0; i < dotCount; i++) dots.push(new Dot());

        const animateDots = () => {
            if (!landingActive) return;
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            dots.forEach(d => {
                d.update();
                d.draw();
            });
            scrollSpeed *= 0.96; // Slightly slower decay for longer trails
            dotAnimationId = requestAnimationFrame(animateDots);
        };
        animateDots();

        // Track global content scroll for dots speed (Increased impact 3x)
        content.addEventListener('scroll', () => {
            scrollSpeed = Math.min(scrollSpeed + 6, 75); // Incremented from 2 -> 6 (3x)
        }, { passive: true });
    }

    // ── GSAP ──
    if (!window.gsap) return;
    if (window.ScrollTrigger) {
        gsap.registerPlugin(ScrollTrigger);
        ScrollTrigger.defaults({ scroller: '#content' });
    }

    const tl = gsap.timeline({ defaults: { ease: 'power4.out' } });

    gsap.set('.lp-title-line', { y: 150, opacity: 0, rotationX: -40 });
    gsap.set(['.lp-intro', '.lp-q', '.lp-ctas'], { y: 50, opacity: 0 });

    tl.to('.lp-title-line', { 
          opacity: 1, y: 0, rotationX: 0, 
          duration: 1.6, stagger: 0.12, ease: 'expo.out' 
      }, 0.2)
      .to('.lp-intro', { opacity: 1, y: 0, duration: 1 }, '-=1.2')
      .to('.lp-q', { opacity: 1, y: 0, duration: 1, stagger: 0.2 }, '-=0.8')
      .to('.lp-ctas', { opacity: 1, y: 0, duration: 1 }, '-=0.8');

    // ── Aggressive Mouse Parallax (Doubled multipliers) ──
    if (content) {
        content.addEventListener('mousemove', (e) => {
            if (!landingActive) return;
            const x = (e.clientX / window.innerWidth - 0.5) * 60;
            const y = (e.clientY / window.innerHeight - 0.5) * 60;
            
            // For particles
            mouse.x = e.clientX;
            mouse.y = e.clientY;

            // Hero parallax (Heavier displacement)
            gsap.to('.lp-title-line', {
                x: (i) => x * (i + 1) * 0.6,
                y: (i) => y * (i + 1) * 0.6,
                duration: 1.5, ease: 'power2.out'
            });

            // Section heading parallax (More visible)
            gsap.to('.lp-section-heading', {
                x: x * 0.4,
                y: y * 0.4,
                duration: 1.8, ease: 'power2.out'
            });
        });
    }

    // Scroll-reveal
    if (window.ScrollTrigger) {
        document.querySelectorAll('[data-gsap-reveal]').forEach(el => {
            gsap.from(el, {
                scrollTrigger: { trigger: el, start: 'top 85%', toggleActions: 'play none none none' },
                opacity: 0, y: 100, duration: 1.4, ease: 'power3.out',
            });
        });

        document.querySelectorAll('.lp-step-v').forEach((el) => {
            gsap.from(el.querySelectorAll('.lp-step-v-num, .lp-step-v-content'), {
                scrollTrigger: { trigger: el, start: 'top 80%', toggleActions: 'play none none none' },
                opacity: 0, x: -80, stagger: 0.3, duration: 1.4, ease: 'power3.out'
            });
        });
    }
}

function destroyLandingAnimations() {
    landingActive = false;
    if (dotAnimationId) cancelAnimationFrame(dotAnimationId);
    if (window.ScrollTrigger) {
        ScrollTrigger.getAll().forEach(t => t.kill());
    }
}

function waitForLibs() {
    return new Promise(resolve => {
        const check = () => {
            if (window.gsap && window.tsParticles) { resolve(); return; }
            setTimeout(check, 80);
        };
        check();
    });
}

/* ---------- routing ---------- */

const pages = {
    '':                 'landing',
    'calibrate':        'calibration',
    'solve':            'solver-min-throws',
    'solve/max-points': 'solver-max-points',
    'solve/min-throws': 'solver-min-throws',
    'solve/min-rounds': 'solver-min-rounds',
};

/** Returns the current route name derived from the URL hash. */
function getRoute() {
    const raw = (location.hash || '').trim();
    const noQuery = raw.split('?')[0];
    const normalized = decodeURIComponent(noQuery)
        .replace(/^#\/?/, '')
        .replace(/\/+$/, '');
    return pages[normalized] ?? 'landing';
}

let currentPage = null;

/**
 * Navigate to the route matching the current URL hash,
 * unmounting the previous page and mounting the new one.
 */
function navigate() {
    const page = getRoute();
    if (page === currentPage) return;

    // unmount previous page
    if (currentPage === 'landing') destroyLandingAnimations();
    if (currentPage === 'calibration') CalibrationPage.unmount();
    if (currentPage?.startsWith('solver-')) SolverPage.unmount();

    // toggle visibility
    document.querySelectorAll('.page').forEach(el => el.classList.remove('active'));
    document.getElementById(`page-${page}`)?.classList.add('active');

    // update nav active state
    document.querySelectorAll('.nav-link').forEach(a => {
        a.classList.toggle('active', a.dataset.page === page);
    });

    currentPage = page;

    // mount new page
    if (page === 'landing') initLandingAnimations();
    if (page === 'calibration' && targetBeds) CalibrationPage.mount(targetBeds, targetBounds);
    if (page.startsWith('solver-') && targetBeds) SolverPage.mount(page, targetBeds, targetBounds);
}

/* ---------- init ---------- */

async function init() {
    State.loadFromStorage();

    const workerFlag = window.DARTS_USE_WORKERS;
    Wasm.setWorkerMode(workerFlag !== false);

    // Ensure nav works even if hashchange is not dispatched by the environment.
    document.querySelectorAll('.nav-link[href^="#/"]').forEach((link) => {
        link.addEventListener('click', (e) => {
            const href = link.getAttribute('href');
            if (!href) return;
            if (link.classList.contains('nav-link-disabled')) {
                e.preventDefault();
                return;
            }
            e.preventDefault();
            if (location.hash !== href) {
                location.hash = href;
            }
            navigate();
        });
    });

    // Navigate immediately (before WASM loads) so the landing page shows
    window.addEventListener('hashchange', navigate);
    navigate();

    // Load WASM
    const statusEl = document.getElementById('wasm-status');
    try {
        await Wasm.init();
        await Wasm.loadTarget('target.out');

        // Parse target geometry for JS rendering
        const content = Wasm.getTargetContent();
        targetBeds = parseTarget(content);
        targetBounds = computeBounds(targetBeds);

        statusEl.title   = '✓ WASM Ready';
        statusEl.className = 'wasm-dot ready';

        // Re-mount current page now that data is available
        if (currentPage === 'calibration') CalibrationPage.mount(targetBeds, targetBounds);
        if (currentPage?.startsWith('solver-')) SolverPage.mount(currentPage, targetBeds, targetBounds);

    } catch (err) {
        console.error('WASM init failed:', err);
        statusEl.title   = '✗ WASM Error';
        statusEl.className = 'wasm-dot error';
    }
}

init();
