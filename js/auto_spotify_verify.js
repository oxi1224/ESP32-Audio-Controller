import open from 'open';
import fs from 'fs/promises'

(async () => {
  const ENV = new Map();
  const f = await fs.open("../.env");
  if (!f) {
    console.log("Failed to open .env file");
    return;
  }
  for await (const line of /** @type {string[]} line*/ (f.readLines())) {
    const key = line.substring(0, line.indexOf('=')); 
    const value = line.substring(line.indexOf('"') + 1, line.lastIndexOf('"'));
    ENV.set(key, value);
  }
  await f.close();
  if (!ENV.has("VERIFY_URL")) {
    console.log(".env file does not contain VERIFY_URL");
  }
  await new Promise(r => setTimeout(r, 500));
  open(ENV.get("VERIFY_URL"));
})(); 