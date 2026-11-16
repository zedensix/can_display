CREATE TABLE `samples` (
	`session` integer NOT NULL,
	`t` integer NOT NULL,
	`rpm` real,
	`coolant` real,
	`oil` real,
	`intake` real,
	`afr` real,
	`battery` real,
	`ethanol` real,
	PRIMARY KEY(`session`, `t`)
);
--> statement-breakpoint
CREATE TABLE `sessions` (
	`id` integer PRIMARY KEY NOT NULL,
	`received_at` integer NOT NULL
);
