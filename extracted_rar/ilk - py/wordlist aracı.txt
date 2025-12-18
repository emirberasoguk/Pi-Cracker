import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext
from PIL import Image, ImageTk
import random
import os
import subprocess
import webbrowser
import zipfile

# === BANNER ===
def banner():
    print(r"""
       🔥  SİBER TİM İŞTEE !
           
        (\_/)        
       ( •_•)   ← Tavşan ama root yetkili...
       />🍪   ⌐■_■   "TAVŞANI DEĞİL SİBERTİMİ TAKİP ET !"

    ╔════════════════════════════════════════════════════╗h
    ║     ☠ YA BENİMSİN YA HURDACININ ☠     ║
    ╚════════════════════════════════════════════════════╝
    """)

# === LEETSPEAK & VARYASYONLAR ===
def leetspeak(word):
    leet_dict = {'a': '@', 'e': '3', 'i': '1', 's': '$', 'o': '0', 'b': '8'}
    return ''.join(leet_dict.get(c.lower(), c) for c in word)

def generate_smart_variations(keyword, numbers, symbols, extra_words):
    variations = set()
    base = [keyword, keyword.lower(), keyword.upper(), keyword.capitalize(), leetspeak(keyword), keyword[::-1]]
    numbers = [n.strip() for n in numbers.split(',') if n.strip()]
    symbols = list(symbols)
    extras = [e.strip() for e in extra_words.split(',') if e.strip()]
    for b in base:
        variations.add(b)
        for num in numbers:
            variations.update([f"{b}{num}", f"{num}{b}"])
        for sym in symbols:
            variations.update([f"{b}{sym}", f"{sym}{b}"])
        for ext in extras:
            variations.update([f"{b}{ext}", f"{ext}{b}"])
        for num in numbers:
            for sym in symbols:
                variations.update([f"{b}{sym}{num}", f"{sym}{b}{num}", f"{num}{b}{sym}"])
    return variations

# === TXT KAYDET ===
def save_to_file():
    filepath = filedialog.asksaveasfilename(
        defaultextension=".txt",
        initialfile="SIBERTIM_Wordlist.txt",
        filetypes=[("Metin Dosyası", "*.txt")]
    )
    if filepath:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(output.get(1.0, tk.END))
        messagebox.showinfo("Kaydedildi", f"Wordlist kaydedildi:\n{filepath}")
        try:
            os.startfile(filepath)  # Windows
        except AttributeError:
            subprocess.call(['open', filepath])  # macOS

# === ZIP KAYDET ===
def save_to_zip():
    content = output.get(1.0, tk.END).strip()
    if not content:
        messagebox.showerror("Hata", "Kaydedilecek içerik yok.")
        return

    filepath = filedialog.asksaveasfilename(
        defaultextension=".zip",
        initialfile="SIBERTIM_Wordlist.zip",
        filetypes=[("ZIP Dosyası", "*.zip")]
    )
    if filepath:
        try:
            temp_txt_name = "wordlist.txt"
            with open(temp_txt_name, "w", encoding="utf-8") as f:
                f.write(content)
            with zipfile.ZipFile(filepath, 'w', zipfile.ZIP_DEFLATED) as zipf:
                zipf.write(temp_txt_name)
            os.remove(temp_txt_name)
            messagebox.showinfo("Kaydedildi", f"ZIP dosyası oluşturuldu:\n{filepath}")
            try:
                os.startfile(filepath)  # Windows
            except AttributeError:
                subprocess.call(['open', filepath])  # macOS
        except Exception as e:
            messagebox.showerror("Hata", f"Zip kaydı sırasında hata:\n{e}")

# === SİTE BUTONU ===
def open_website():
    webbrowser.open("https://sibertim.com")

def generate_wordlist():
    output.delete(1.0, tk.END)
    kw = keyword_entry.get()
    nums = number_entry.get()
    syms = symbol_entry.get()
    extras = extra_entry.get()
    if not kw:
        messagebox.showerror("Hata", "Anahtar kelime boş olamaz.")
        return
    result = generate_smart_variations(kw, nums, syms, extras)
    for word in result:
        output.insert(tk.END, word + "\n")
    messagebox.showinfo("Tamamlandı", f"{len(result)} şifre oluşturuldu.")

# === GUI ===
root = tk.Tk()
root.title("SİBERTİM PRO - Akıllı Wordlist Üretici")
root.geometry("1200x850")
root.configure(bg="#0f0f0f")

canvas = tk.Canvas(root, width=1200, height=850, bg="#0f0f0f", highlightthickness=0)
canvas.place(x=0, y=0)

# === MATRIX EFEKT ===
class MatrixRain:
    def __init__(self, canvas, width, height, font_size=16):
        self.canvas = canvas
        self.width = width
        self.height = height
        self.font_size = font_size
        self.columns = int(self.width / self.font_size)
        self.drops = [0 for _ in range(self.columns)]

    def draw(self):
        self.canvas.delete("matrix")
        for i in range(len(self.drops)):
            char = random.choice(['0', '1'])
            x = i * self.font_size
            y = self.drops[i] * self.font_size
            self.canvas.create_text(x, y, text=char, fill='#00FF00',
                                    font=('Courier', self.font_size, 'bold'),
                                    tags='matrix', anchor='nw')
            if y > self.height and random.random() > 0.9:
                self.drops[i] = 0
            else:
                self.drops[i] += 1
        self.canvas.after(30, self.draw)

rain = MatrixRain(canvas, 1200, 850)
rain.draw()

# === LOGO ===
try:
    # Bu satır çalışılan dizini .py dosyasının olduğu yere sabitler
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    logo_img = Image.open("logonet.png").resize((200, 200))
    logo_photo = ImageTk.PhotoImage(logo_img)
    logo_label = tk.Label(root, image=logo_photo, bg="#0f0f0f")
    logo_label.image = logo_photo
    logo_label.place(x=500, y=20)
except Exception as e:
    print("Logo yüklenemedi:", e)

# === FORM ALANI ===
form = tk.Frame(root, bg="#0f0f0f")
form.place(x=400, y=240)

def field(label_text):
    tk.Label(form, text=label_text, fg="#00FF00", bg="#0f0f0f", font=("Courier", 12)).pack()
    e = tk.Entry(form, width=50, bg="#1a1a1a", fg="#00FF00", insertbackground="#00FF00", font=("Courier", 11))
    e.pack(pady=6)
    return e

keyword_entry = field("Anahtar Kelime:")
number_entry = field("Sayılar (virgülle):")
symbol_entry = field("Semboller (örn: !@.+):")
extra_entry = field("Ekstra Kelimeler (örn: admin,test):")

# === BUTONLAR ===
tk.Button(form, text="Varyasyonları Oluştur", command=generate_wordlist,
          bg="#00FF00", fg="black", font=("Courier", 12, "bold")).pack(pady=12)

tk.Button(form, text="TXT Olarak Kaydet", command=save_to_file,
          bg="#1aff66", fg="black", font=("Courier", 12, "bold")).pack(pady=6)

tk.Button(form, text="ZIP Olarak Kaydet", command=save_to_zip,
          bg="#ffaa00", fg="black", font=("Courier", 12, "bold")).pack(pady=6)

tk.Button(form, text="SIBERTIM.COM", command=open_website,
          bg="#00ccff", fg="black", font=("Courier", 12, "bold")).pack(pady=6)

# === SONUÇ ALANI ===
output = scrolledtext.ScrolledText(root, width=140, height=12, bg="black",
                                   fg="#00FF00", insertbackground="green", font=("Courier", 11))
output.place(x=30, y=670)

# === BAŞLAT ===
if __name__ == "__main__":
    banner()
    root.mainloop()
