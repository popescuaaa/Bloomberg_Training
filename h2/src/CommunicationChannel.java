import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Semaphore;

/**
 * Class that implements the channel used by headquarters and space explorers to communicate.
 */
public class CommunicationChannel {
	/**
	 *  Messages queues.
	 */
	private Queue<Message> messagesSpaceExplorers;
	private Queue<Message> messagesHeadQuarters;

	/**
	 *  Semaphores for accessing the queues.
	 */
	public Semaphore messagesSpaceExplorersSemaphore;
	public Semaphore messagesHeadQuartersSemaphore;
	public Semaphore canReadSpaceExplorersFromHeadQuarter;

	/**
	 *  Internal semaphores to maintain order in messages
	 *  during the process of double messages posting performed
	 *  by the HeadQuarters.
	 */
	public Semaphore accessChannelAsAHeadQuarter;

	/**
	 *  Internal data to locate the current thread that access
	 *  the methods.
	 */
	private ArrayList<Integer> solarSystemIDs;
	private ConcurrentHashMap<Long, ArrayList<Message>> waitingToBePosted;


	/**
	 * Creates a {@code CommunicationChannel} object.
	 */
	public CommunicationChannel() {
		messagesSpaceExplorers = new LinkedList<>();
		messagesHeadQuarters = new LinkedList<>();
		messagesHeadQuartersSemaphore = new Semaphore(1);
		messagesSpaceExplorersSemaphore = new Semaphore(1);
		canReadSpaceExplorersFromHeadQuarter = new Semaphore(0);
		accessChannelAsAHeadQuarter = new Semaphore(1);
		solarSystemIDs = new ArrayList<>();
		/**
		 *  I considered the capacity of the hash map big enough
		 *  in order not to have problems concerning the number of
		 *  threads and the fact that java Thread.currentThared().getID ->
		 *  return a uniq identifier in system
		 */
		waitingToBePosted = new ConcurrentHashMap<>(100000);
		for (int i = 0; i < 100000; i++) {
			waitingToBePosted.put((long) i, new ArrayList<>(2));
		}
	}

	/**
	 * Puts a message on the space explorer channel (i.e., where space explorers write to and 
	 * headquarters read from).
	 * 
	 * @param message
	 *            message to be put on the channel
	 */
	public void putMessageSpaceExplorerChannel(Message message) {
		try {
			messagesSpaceExplorersSemaphore.acquire();
			messagesSpaceExplorers.add(message);
			messagesSpaceExplorersSemaphore.release();
		} catch (InterruptedException e) {

		}
	}

	/**
	 * Gets a message from the space explorer channel (i.e., where space explorers write to and
	 * headquarters read from).
	 * 
	 * @return message from the space explorer channel
	 */
	public Message getMessageSpaceExplorerChannel()  {
		try {
			messagesSpaceExplorersSemaphore.acquire();
			Message toSend =  messagesSpaceExplorers.poll();
			messagesSpaceExplorersSemaphore.release();
			return toSend;
		} catch (InterruptedException e) {

		}
		return null;
	}

	/**
	 * Puts a message on the headquarters channel (i.e., where headquarters write to and 
	 * space explorers read from).
	 * 
	 * @param message
	 *            message to be put on the channel
	 *
	 *
	 *  The idea here is that the communication channel
	 *  will be accessed by headQuarters one by one and
	 *  the EXIT and END message will be dropped as the
	 *  Space Explorer will try to get a semaphore that
	 *  that has a value (at the end of each process )
	 *  smaller that 0 so -> the try catch structure will be
	 *  triggered and the process will be stopped.
	 */
	public void putMessageHeadQuarterChannel(Message message) {
		try {
			accessChannelAsAHeadQuarter.acquire();
			Long currentThreadID = Thread.currentThread().getId();
			String data = message.getData();
			Integer currentSolarSystem = message.getCurrentSolarSystem();

			if (data != HeadQuarter.END) {
				if (data != HeadQuarter.EXIT) {
					if (waitingToBePosted.get(currentThreadID).size() == 0) {
						waitingToBePosted.get(currentThreadID).add(message);
					} else {
						if (!solarSystemIDs.contains(currentSolarSystem)) {
							/**
							 *  In this point the Channel is sure that
							 *  the specific HQ has two messages and it can
							 *  post on space explorers queue.
							 *
							 */
							messagesHeadQuartersSemaphore.acquire();
							messagesHeadQuarters.add(waitingToBePosted.get(currentThreadID).get(0));
							messagesHeadQuarters.add(message);
							solarSystemIDs.add(currentSolarSystem);
							canReadSpaceExplorersFromHeadQuarter.release();
							messagesHeadQuartersSemaphore.release();
						}
						waitingToBePosted.get(currentThreadID).remove(0);
					}
				}
			}

			accessChannelAsAHeadQuarter.release();
		} catch (InterruptedException e) {

		}


	}

	/**
	 * Gets a message from the headquarters channel (i.e., where headquarters write to and
	 * space explorer read from).
	 * 
	 * @return message from the header quarter channel
	 */
	public Message getMessageHeadQuarterChannel() {
		try {
			messagesHeadQuartersSemaphore.acquire();
			Message toSend =  messagesHeadQuarters.poll();
			messagesHeadQuartersSemaphore.release();
			return toSend;
		} catch (InterruptedException e) {

		}

		return null;
	}

}
