import memoryCache from 'memory-cache';
import { Request, Response, NextFunction } from 'express';

const ITS_BEEN__ONE_WEEK_SINCE_YOU_LOOKED_AT_ME = 1000 * 60 * 60 * 24 * 7;

export function cacheMiddleware(req: Request, res: Response, next: NextFunction) {
  const key = req.originalUrl || req.url;
  const cached = memoryCache.get(key);
  if (cached) return res.send(cached);
  const origSend = res.send.bind(res);
  res.send = (contents) => {
    if (res.statusCode === 200) {
      memoryCache.put(key, contents, ITS_BEEN__ONE_WEEK_SINCE_YOU_LOOKED_AT_ME);
    }
    return origSend(contents);
  };
  next();
}

export const invalidateCache = memoryCache.del;
export const clearCache = memoryCache.clear;
