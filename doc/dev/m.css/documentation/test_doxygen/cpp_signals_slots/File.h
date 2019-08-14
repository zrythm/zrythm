/** @file
 * @brief Uh
 */

/** @brief A class */
class Class {
    signals:
        /**
         * @brief A signal
         *
         * Details.
         */
        void signal();

    public slots:
        /**
         * @brief A public slot
         *
         * Details.
         */
        void publicSlot();

    protected slots:
        /**
         * @brief A protected slot
         *
         * Details.
         */
        void protectedSlot();

    private slots:
        /**
         * @brief A private virtual slot
         *
         * Shown because it's virtual.
         */
        virtual void privateVirtualSlot();

        /**
         * @brief A private slot
         *
         * Non-virtual, so hidden.
         */
        void privateSlot();

    /** @{ @name A user-defined group */

    signals:
        /**
         * @brief A grouped signal
         *
         * Details.
         */
        void groupedSignal();

    public slots:
        /**
         * @brief A public grouped slot
         *
         * Details.
         */
        void publicGroupedSlot();

    protected slots:
        /**
         * @brief A protected grouped slot
         *
         * Details.
         */
        void protectedGroupedSlot();

    private slots:
        /**
         * @brief A private virtual grouped slot
         *
         * Shown because it's virtual.
         */
        virtual void privateVirtualGroupedSlot();

        /**
         * @brief A private grouped slot
         *
         * Non-virtual, so hidden.
         */
        void privateGroupedSlot();

    /*@}*/
};
