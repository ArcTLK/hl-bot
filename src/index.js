const Telegram = require('telegram-bot-api');
const sqlite3 = require('sqlite3');
const dotenv = require('dotenv');
const db = new sqlite3.Database('bot.db');
dotenv.config();

// create table 
db.run(`CREATE TABLE IF NOT EXISTS users (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	telegramId TEXT NOT NULL UNIQUE,
	name TEXT NOT NULL
)`);

const api = new Telegram({
	token: process.env.BOT_TOKEN
});

const messageProvider = new Telegram.GetUpdateMessageProvider();

api.setMessageProvider(messageProvider);
api.start()
.then(() => {
	console.log('Bot is now running!');
})
.catch(console.error);

api.on('update', update => {
	try {
		if (update.message && update.message.text) {
			// replace bot name
			update.message.text = update.message.text.replace(process.env.BOT_NAME, '');
			if (update.message.text === '/hi') {
				api.sendMessage({
					chat_id: update.message.chat.id,
					text: 'YOLO!'
				});
			}
			else if (update.message.text === '/spam') {
				// message everyone
				db.all('SELECT telegramId, name FROM users', [], (err, rows) => {
					if (err) {
						api.sendMessage({
							chat_id: update.message.chat.id,
							text: `Error: ${JSON.stringify(err)}`
						}).catch(console.error);
					}
					else {
						const names = rows.map(x => `[${x.name}](tg://user?id=${x.telegramId})`);
						api.sendMessage({
							chat_id: update.message.chat.id,
							text: `*${update.message.from.first_name}* wants to play!\nJoin up ppl!\n\n${names.join('\n')}`,
							parse_mode: 'Markdown'
						}).catch(console.error);
					}
				});
			}
			else if (update.message.text === '/hl') {
				db.run('INSERT INTO users (telegramId, name) VALUES (?, ?)', [
					update.message.from.id,
					update.message.from.first_name
				], err => {
					if (err) {
						api.sendMessage({
							chat_id: update.message.chat.id,
							text: `Error: ${JSON.stringify(err)}`
						}).catch(console.error);
					}
					else {
						api.sendMessage({
							chat_id: update.message.chat.id,
							text: `${update.message.from.first_name} has been added to the HL List!`
						}).catch(console.error);
					}
				});
			}
			else if (update.message.text === '/nohl') {
				db.run('DELETE FROM users WHERE telegramId = ?', [
					update.message.from.id
				], err => {
					if (err) {
						api.sendMessage({
							chat_id: update.message.chat.id,
							text: `Error: ${JSON.stringify(err)}`
						}).catch(console.error);
					}
					else {
						api.sendMessage({
							chat_id: update.message.chat.id,
							text: `${update.message.from.first_name} has been removed from the HL List!`
						}).catch(console.error);
					}
				});
			}
		}
	}
	catch (err) {
		console.error(err);
		api.sendMessage({
			chat_id: update.message.chat.id,
			text: `Error: ${JSON.stringify(err)}`
		}).catch(console.error);
	}
});
