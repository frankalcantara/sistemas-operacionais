/**
 * Funcionalidades comuns para documentos HTML
 * Sistema de navegação, TOC móvel e animações
 */

// Mobile TOC Toggle
function toggleTOC() {
    const toc = document.getElementById('toc');
    if (toc) {
        toc.classList.toggle('open');
    }
}

// Inicialização principal quando DOM estiver carregado
document.addEventListener('DOMContentLoaded', function() {
    initializeTOC();
    initializeSmoothScrolling();
    initializeFadeAnimations();
    initializeClickOutside();
    initializeResize();
    initializeHighlightJS();
});

// Inicializar funcionalidades do TOC
function initializeTOC() {
    const links = document.querySelectorAll('a[href^="#"]');
    const sections = document.querySelectorAll('section[id]');
    
    // Smooth scrolling para links âncora
    links.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            const targetId = this.getAttribute('href').substring(1);
            const targetSection = document.getElementById(targetId);
            if (targetSection) {
                const offset = 80;
                const elementPosition = targetSection.getBoundingClientRect().top + window.pageYOffset;
                window.scrollTo({
                    top: elementPosition - offset,
                    behavior: 'smooth'
                });
            }
            // Fechar TOC móvel após clique
            if (window.innerWidth <= 1024) {
                const toc = document.getElementById('toc');
                if (toc) {
                    toc.classList.remove('open');
                }
            }
        });
    });
    
    // Highlight ativo no TOC baseado na seção visível
    if (sections.length > 0) {
        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    const id = entry.target.getAttribute('id');
                    links.forEach(link => {
                        link.classList.remove('bg-light', 'text-dark');
                        if (link.getAttribute('href') === `#${id}`) {
                            link.classList.add('bg-light', 'text-dark');
                        }
                    });
                }
            });
        }, {
            rootMargin: '-80px 0px -70% 0px'
        });
        
        sections.forEach(section => {
            observer.observe(section);
        });
    }
}

// Inicializar smooth scrolling
function initializeSmoothScrolling() {
    // Já implementado em initializeTOC()
    // Mantido como função separada para clareza organizacional
}

// Inicializar animações de fade-in
function initializeFadeAnimations() {
    const fadeElements = document.querySelectorAll('.fade-in');
    
    if (fadeElements.length > 0) {
        const fadeObserver = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    entry.target.style.opacity = '1';
                    entry.target.style.transform = 'translateY(0)';
                }
            });
        }, {
            threshold: 0.1
        });
        
        fadeElements.forEach(el => {
            el.style.opacity = '0';
            el.style.transform = 'translateY(20px)';
            el.style.transition = 'opacity 0.8s ease, transform 0.8s ease';
            fadeObserver.observe(el);
        });
    }
}

// Inicializar fechamento do TOC ao clicar fora (mobile)
function initializeClickOutside() {
    document.addEventListener('click', function(event) {
        const toc = document.getElementById('toc');
        const hamburger = document.querySelector('button[onclick="toggleTOC()"]');
        
        if (toc && hamburger && window.innerWidth <= 1024 && toc.classList.contains('open')) {
            if (!toc.contains(event.target) && event.target !== hamburger && !hamburger.contains(event.target)) {
                toc.classList.remove('open');
            }
        }
    });
}

// Inicializar tratamento de redimensionamento
function initializeResize() {
    window.addEventListener('resize', function() {
        const toc = document.getElementById('toc');
        if (toc && window.innerWidth > 1024) {
            toc.classList.remove('open');
        }
    });
}

// Funções utilitárias adicionais que podem ser úteis em outros documentos

// Scroll suave para elemento específico
function scrollToElement(elementId, offset = 80) {
    const element = document.getElementById(elementId);
    if (element) {
        const elementPosition = element.getBoundingClientRect().top + window.pageYOffset;
        window.scrollTo({
            top: elementPosition - offset,
            behavior: 'smooth'
        });
    }
}

// Verificar se elemento está visível na viewport
function isElementInViewport(element) {
    const rect = element.getBoundingClientRect();
    return (
        rect.top >= 0 &&
        rect.left >= 0 &&
        rect.bottom <= (window.innerHeight || document.documentElement.clientHeight) &&
        rect.right <= (window.innerWidth || document.documentElement.clientWidth)
    );
}

// Debounce para otimizar eventos de scroll/resize
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

// Adicionar classe ao body quando TOC móvel estiver aberto (para CSS adicional se necessário)
function updateBodyClass() {
    const toc = document.getElementById('toc');
    if (toc) {
        if (toc.classList.contains('open')) {
            document.body.classList.add('toc-open');
        } else {
            document.body.classList.remove('toc-open');
        }
    }
}

// Observar mudanças no TOC para atualizar classe do body
if (typeof MutationObserver !== 'undefined') {
    document.addEventListener('DOMContentLoaded', function() {
        const toc = document.getElementById('toc');
        if (toc) {
            const observer = new MutationObserver(function(mutations) {
                mutations.forEach(function(mutation) {
                    if (mutation.type === 'attributes' && mutation.attributeName === 'class') {
                        updateBodyClass();
                    }
                });
            });
            
            observer.observe(toc, {
                attributes: true,
                attributeFilter: ['class']
            });
        }
    });
}

// Inicializar Highlight.js para syntax highlighting
function initializeHighlightJS() {
    // Aguardar que o hljs seja carregado
    if (typeof hljs === 'undefined') {
        console.warn('Highlight.js não está disponível ainda, tentando novamente em 100ms...');
        setTimeout(initializeHighlightJS, 100);
        return;
    }
    
    try {
        // Configurar Highlight.js
        hljs.configure({
            ignoreUnescapedHTML: true,
            throwUnescapedHTML: false
        });
        
        // Aplicar highlighting a todos os blocos de código
        hljs.highlightAll();
        
        // Log para debug
        const codeBlocks = document.querySelectorAll('pre code');
        console.log(`Highlight.js inicializado com sucesso. Encontrados ${codeBlocks.length} blocos de código.`);
        
        // Verificar se as classes foram aplicadas
        codeBlocks.forEach((block, index) => {
            console.log(`Bloco ${index + 1}:`, {
                hasHljsClass: block.classList.contains('hljs'),
                languageClass: Array.from(block.classList).find(c => c.startsWith('language-')),
                innerHTML: block.innerHTML.substring(0, 50) + '...'
            });
        });
        
    } catch (error) {
        console.error('Erro ao inicializar Highlight.js:', error);
    }
}