import readline from "node:readline";

const input = readline.createInterface({
  input: process.stdin,
  crlfDelay: Infinity,
});

for await (const line of input) {
  if (line === "uci") {
    process.stdout.write("id name Fake Catfish\nuci");
    setTimeout(() => process.stdout.write("ok\n"), 2);
  } else if (line === "isready") {
    process.stdout.write("readyok\n");
  } else if (line.startsWith("go ")) {
    process.stdout.write("info string book Ruy Lopez\n");
    process.stdout.write("info depth 3 score cp 24 nodes 120");
    setTimeout(() => process.stdout.write("0 pv e2e4\nbestmove e2e4\n"), 2);
  } else if (line === "quit") {
    process.exit(0);
  }
}
