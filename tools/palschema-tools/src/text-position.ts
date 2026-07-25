export type PositionMapper = (
  offset: number,
  length?: number,
) => { line: number; column: number; offset: number; length: number };

export function createPositionMapper(text: string): PositionMapper {
  const lineStarts = [0];
  for (let index = 0; index < text.length; index += 1) {
    if (text.charCodeAt(index) === 10) {
      lineStarts.push(index + 1);
    }
  }

  return (offset: number, length = 1) => {
    const boundedOffset = Math.max(0, Math.min(offset, text.length));
    let low = 0;
    let high = lineStarts.length;
    while (low + 1 < high) {
      const middle = Math.floor((low + high) / 2);
      if ((lineStarts[middle] ?? 0) <= boundedOffset) {
        low = middle;
      } else {
        high = middle;
      }
    }
    const lineStart = lineStarts[low] ?? 0;
    return {
      line: low + 1,
      column: boundedOffset - lineStart + 1,
      offset: boundedOffset,
      length: Math.max(1, length),
    };
  };
}
