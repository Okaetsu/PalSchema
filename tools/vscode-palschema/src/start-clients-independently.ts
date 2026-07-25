export interface ClientStartFailure<T> {
  candidate: T;
  error: Error;
}

export async function startClientsIndependently<T>(
  candidates: readonly T[],
  start: (candidate: T) => Promise<Error | null>,
): Promise<ClientStartFailure<T>[]> {
  const failures: ClientStartFailure<T>[] = [];
  for (const candidate of candidates) {
    const error = await start(candidate);
    if (error) {
      failures.push({ candidate, error });
    }
  }
  return failures;
}
