// @ts-check
// Note: type annotations allow type checking and IDEs autocompletion

// const lightCodeTheme = require('prism-react-renderer/themes/github');
// const darkCodeTheme = require('prism-react-renderer/themes/dracula');
const themes = require('prism-react-renderer').themes;
const lightCodeTheme = themes.github;
const darkCodeTheme = themes.dracula;

/** @type {import('@docusaurus/types').Config} */
const config = {
	title: 'SkyXEngine',
	tagline: 'платформа разработки игр как модов',
	url: 'https://docs.skyxengine.com',
	baseUrl: '/',
	onBrokenLinks: 'throw',
	onBrokenMarkdownLinks: 'warn',
	favicon: 'img/favicon.ico',

	// GitHub pages deployment config.
	// If you aren't using GitHub pages, you don't need these.
	organizationName: '', // Usually your GitHub org/user name.
	projectName: '', // Usually your repo name.

	// Even if you don't use internalization, you can use this field to set useful
	// metadata like html lang. For example, if your site is Chinese, you may want
	// to replace "en" with "zh-Hans".
	i18n: {
		defaultLocale: 'ru',
		locales: ['ru'],
	},

	presets: [
		[
			'classic',
			/** @type {import('@docusaurus/preset-classic').Options} */
			({
				docs: {
					sidebarPath: require.resolve('./sidebars.js'),
					// Please change this to your repo.
					// Remove this to remove the "edit this page" links.
					editUrl: 'https://dev.ds-servers.com/sip/engine/-/tree/branchX/docs/',
				},
				blog: {
					showReadingTime: true,
					// Please change this to your repo.
					// Remove this to remove the "edit this page" links.
					editUrl: 'https://dev.ds-servers.com/sip/engine/-/tree/branchX/docs/',
				},
				theme: {
					customCss: require.resolve('./src/css/custom.css'),
				},
			}),
		],
	],

	themeConfig:
	/** @type {import('@docusaurus/preset-classic').ThemeConfig} */
	({
		navbar: {
			title: 'SkyXEngine',
			logo: {
				alt: 'SkyXEngine Logo',
				src: 'img/logo.png',
			},
			items: [
				{
					type: 'doc',
					docId: 'intro',
					position: 'left',
					label: 'Reference',
				},
				// {to: '/blog', label: 'Blog', position: 'left'},
				{
					type: 'docsVersionDropdown',
				},
				{
					href: 'https://dev.ds-servers.com/sip/engine',
					label: 'Main repo',
					position: 'right',
				},
				{
					href: 'https://github.com/dogmanet/engine/',
					label: 'GitHub mirror',
					position: 'right',
				},
			],
		},
		footer: {
			style: 'dark',
			links: [
				{
					title: 'Docs',
					items: [
						{
							label: 'Reference',
							to: '/docs/intro',
						},
					],
				},
				{
					title: 'Community',
					items: [
						/*  {
							label: 'Stack Overflow',
							href: 'https://stackoverflow.com/questions/tagged/docusaurus',
						}, */
						/* {
							label: 'Discord',
							href: 'https://discordapp.com/invite/docusaurus',
						}, */
						{
							label: 'VK',
							href: 'https://m.vk.com/project_sip',
						},
					],
				},
				{
					title: 'More',
					items: [
						/* {
							label: 'Blog',
							to: '/blog',
						}, */
						{
							href: 'https://dev.ds-servers.com/sip/engine',
							label: 'Main repo',
						},
						{
							href: 'https://github.com/dogmanet/engine/',
							label: 'GitHub mirror',
						},
					],
				},
			],
			copyright: `Copyright © ${new Date().getFullYear()} SkyXEngine.com. Built with Docusaurus.`,
		},
		prism: {
			theme: lightCodeTheme,
			darkTheme: darkCodeTheme,
		},
	}),
};

module.exports = config;
