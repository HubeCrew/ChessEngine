/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      fontFamily: {
        sans: ["Geist", "Satoshi", "Aptos", "Segoe UI", "system-ui", "sans-serif"],
        mono: ["JetBrains Mono", "Cascadia Mono", "SFMono-Regular", "ui-monospace", "monospace"]
      },
      colors: {
        ink: "#18191b",
        coal: "#202326",
        line: "#d9ded8",
        moss: "#5f8f7c",
        paper: "#f5f5f1"
      },
      boxShadow: {
        diffusion: "0 24px 70px -38px rgba(32,35,38,0.48)"
      }
    }
  },
  plugins: []
};
