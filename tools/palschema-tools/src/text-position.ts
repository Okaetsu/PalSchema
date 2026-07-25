export function positionAt(
  text: string,
  offset: number,
  length = 1,
): { line: number; column: number; offset: number; length: number } {
  const boundedOffset = Math.max(0, Math.min(offset, text.length));
  let line = 1;
  let lastLineStart = 0;

  for (let index = 0; index < boundedOffset; index += 1) {
    if (text.charCodeAt(index) === 10) {
      line += 1;
      lastLineStart = index + 1;
    }
  }

  return {
    line,
    column: boundedOffset - lastLineStart + 1,
    offset: boundedOffset,
    length: Math.max(1, length),
  };
}
