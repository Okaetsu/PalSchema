import { isAbsolute, relative, resolve, sep } from "node:path";

export function isPathContained(root: string, candidate: string): boolean {
  const pathFromRoot = relative(root, candidate);
  return (
    pathFromRoot !== "" &&
    pathFromRoot !== ".." &&
    !pathFromRoot.startsWith(`..${sep}`) &&
    !isAbsolute(pathFromRoot)
  );
}

export function resolveContainedPath(
  root: string,
  file: string,
  label: string,
): string {
  const candidate = resolve(root, file);
  if (!isPathContained(root, candidate)) {
    throw new Error(`${label}: ${file}`);
  }
  return candidate;
}
