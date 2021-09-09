import fs from 'fs';
import path from 'path';
import urgentMessage from './urgent-message';

type Tokens = { [key: string]: string };

const templates: { [key: string]: string } = {};
fs.readdirSync(path.join(__dirname, '../../templates')).forEach((fileName) => {
  const contents = fs.readFileSync(path.join(__dirname, '../../templates', fileName), 'utf-8');
  templates[fileName.split('.').slice(0, -1).join('')] = contents;
});

export function renderTemplate(template: string, tokens?: Tokens) {
  if (!(template in templates)) throw new Error(`Template not found: ${template}`);
  let buf = templates[template];
  if (tokens) {
    Object.keys(tokens).forEach((key) => {
      const regexp = new RegExp(`{{${key}}}`, 'g');
      buf = buf.replace(regexp, tokens[key]);
    });
  }
  return buf;
};

export function renderPage(template: string, title?: string, tokens?: Tokens) {
  return renderTemplate('base', {
    title: `Beat Saber Quest Crash Logs${title ? ` | ${title}` : ''}`,
    page: renderTemplate(template, tokens),
    urgentMessage: urgentMessage(),
  });
}

