# WSL 開發環境設定

本專案統一使用 Ubuntu on WSL 2 開發與驗證 C、Go、Rust。C 版本以
POSIX Socket 為基準，因此 Linux 環境也是專案實作邊界的一部分。

## 1. 安裝 Ubuntu on WSL 2

以系統管理員身分開啟 PowerShell：

```powershell
wsl --install -d Ubuntu
```

若 Windows 要求重新啟動，完成重啟後再開啟 Ubuntu。第一次啟動時，
依畫面提示建立 Linux username 與 password。這組密碼會在使用 `sudo`
時用到；輸入密碼時畫面不顯示任何字元是正常行為。

在 PowerShell 確認 distribution 與 WSL 版本：

```powershell
wsl --list --verbose
wsl --set-default Ubuntu
```

進入 Ubuntu 後，確認目前不是以 `root` 登入：

```bash
whoami
```

## 2. 安裝 C 工具鏈與共用工具

在 Ubuntu 執行：

```bash
sudo apt update
sudo apt install -y build-essential git curl ca-certificates
```

確認工具可用：

```bash
gcc --version
make --version
git --version
curl --version
```

`build-essential` 會安裝 GCC、Make 與 C 開發所需的基本工具。

## 3. 安裝 Go

安裝 Ubuntu 套件庫提供的 Go：

```bash
sudo apt install -y golang-go
```

確認版本與環境位置：

```bash
go version
go env GOPATH GOROOT
```

目前 `go/go.mod` 宣告 Go 1.22，因此需要 Go 1.22 或更新版本。

## 4. 安裝 Rust

使用 Rust 官方建議的 `rustup` 安裝方式：

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

安裝選單使用預設選項，完成後讓目前的 shell 載入 Cargo 路徑：

```bash
source "$HOME/.cargo/env"
```

確認工具鏈：

```bash
rustc --version
cargo --version
rustup --version
```

## 5. 將專案複製到 Linux 檔案系統

建議把 working copy 放在 WSL 的 home directory，而不是 `/mnt/c`。
這可避免跨檔案系統的權限、效能與工具行為差異。

```bash
mkdir -p ~/projects
cd ~/projects
git clone https://github.com/zrkluke/raw-http-server.git
cd raw-http-server
```

確認位置與 Git 狀態：

```bash
pwd
git status
ls
```

預期 `pwd` 顯示 `/home/<username>/projects/raw-http-server`，並可看到
`c`、`go`、`rust`、`openspec` 與 `README.md`。

## 6. 驗證三種語言的專案骨架

在 repository 根目錄執行：

```bash
make -C c

cd go
go run .
cd ..

cd rust
cargo run
cd ..
```

目前三個骨架程式刻意不輸出內容。命令正常結束且沒有 compiler error，
就代表驗證成功。

最後檢查 Git 狀態：

```bash
git status
```

`c/raw-http-server-c` 與 `rust/target/` 是 build artifacts，已由
`.gitignore` 排除。`rust/Cargo.lock` 會提交至 Git，因為本專案是 Rust
application，需要可重現的 dependency resolution。

## 疑難排解

### WSL 進入 Docker Desktop 而非 Ubuntu

在 PowerShell 將 Ubuntu 設為預設 distribution，再明確開啟它：

```powershell
wsl --set-default Ubuntu
wsl --distribution Ubuntu
```

### Ubuntu 預設以 root 登入

確認 Ubuntu 的 `/etc/wsl.conf` 包含預期的 Linux username：

```ini
[user]
default=<username>
```

接著回到 PowerShell 重啟 WSL：

```powershell
wsl --shutdown
wsl --distribution Ubuntu
```

請將 `<username>` 換成初始化 Ubuntu 時建立的帳號。

### 終端機顏色難以閱讀

可直接從 Windows 開始選單開啟 Ubuntu，或安裝 Windows Terminal 後選擇
Ubuntu profile。更換終端機程式不會改變 WSL 工具鏈或專案檔案。
