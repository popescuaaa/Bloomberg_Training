import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Set;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Semaphore;

/**
 * Class for a space explorer.
 */
public class SpaceExplorer extends Thread {
	/**
	 *  Internal data for constructor
	 */
	private Integer hashCount;
	private Set<Integer> discovered;
	private CommunicationChannel communicationChannel;

	/**
	 *  Semaphores for accessing queues
	 */
	public static Semaphore getMessageFromHeadQuartersQueue;
	public static Semaphore postMessageInSpaceExplorersQueue;


	/**
	 * Creates a {@code SpaceExplorer} object.
	 * 
	 * @param hashCount
	 *            number of times that a space explorer repeats the hash operation
	 *            when decoding
	 * @param discovered
	 *            set containing the IDs of the discovered solar systems
	 * @param channel
	 *            communication channel between the space explorers and the
	 *            headquarters
	 */
	public SpaceExplorer(Integer hashCount, Set<Integer> discovered, CommunicationChannel channel) {
		this.communicationChannel = channel;
		this.discovered = discovered;
		this.hashCount = hashCount;
		getMessageFromHeadQuartersQueue = new Semaphore(1);
		postMessageInSpaceExplorersQueue = new Semaphore(1);
	}

	@Override
	public void run() {
		/**
		 *  The try catch structure will act like a controller
		 *  for the execution flow and eventually it will stop the
		 *  space explorer process.
		 *
		 */
		try {
		while (true) {
				getMessageFromHeadQuartersQueue.acquire();
				communicationChannel.canReadSpaceExplorersFromHeadQuarter.acquire();
				Message parentSolarSystemInfo = communicationChannel.getMessageHeadQuarterChannel();
				Message currentSolarSystemInfo = communicationChannel.getMessageHeadQuarterChannel();
				getMessageFromHeadQuartersQueue.release();
				/**
			 	*  The decryption step is the only part made in parallel.
			 	*
			 	*/
				String decryptedResult = encryptMultipleTimes(currentSolarSystemInfo.getData(), hashCount);
				postMessageInSpaceExplorersQueue.acquire();

				communicationChannel.putMessageSpaceExplorerChannel(
					new Message(
							parentSolarSystemInfo.getCurrentSolarSystem(),
							currentSolarSystemInfo.getCurrentSolarSystem(),
							decryptedResult
					)
				);
				postMessageInSpaceExplorersQueue.release();
			}
		} catch (InterruptedException e) {

		}
	}


	
	/**
	 * Applies a hash function to a string for a given number of times (i.e.,
	 * decodes a frequency).
	 * 
	 * @param input
	 *            string to he hashed multiple times
	 * @param count
	 *            number of times that the string is hashed
	 * @return hashed string (i.e., decoded frequency)
	 */
	private String encryptMultipleTimes(String input, Integer count) {
		String hashed = input;
		for (int i = 0; i < count; ++i) {
			hashed = encryptThisString(hashed);
		}

		return hashed;
	}

	/**
	 * Applies a hash function to a string (to be used multiple times when decoding
	 * a frequency).
	 * 
	 * @param input
	 *            string to be hashed
	 * @return hashed string
	 */
	private static String encryptThisString(String input) {
		try {
			MessageDigest md = MessageDigest.getInstance("SHA-256");
			byte[] messageDigest = md.digest(input.getBytes(StandardCharsets.UTF_8));

			// convert to string
			StringBuffer hexString = new StringBuffer();
			for (int i = 0; i < messageDigest.length; i++) {
				String hex = Integer.toHexString(0xff & messageDigest[i]);
				if (hex.length() == 1)
					hexString.append('0');
				hexString.append(hex);
			}
			return hexString.toString();

		} catch (NoSuchAlgorithmException e) {
			throw new RuntimeException(e);
		}
	}
}
