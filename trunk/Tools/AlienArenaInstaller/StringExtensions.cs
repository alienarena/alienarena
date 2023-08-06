using System;

namespace AlienArenaInstaller
{
	public static class StringExtensions
	{
		public static int ToInteger(this String text)
		{
			int.TryParse(text, out int intValue);
			return intValue;
		}
	}
}
